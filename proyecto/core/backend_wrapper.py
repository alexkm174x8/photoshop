"""
backend_wrapper.py 
==============================================================
Que hace:
    - Recibe un ProcessingRequest que esta construido por MainWindow.
    - Lo traduce a una línea de terminal para el ejecutable C.
    - Lanza el proceso, captura su stdout y parsea el tiempo de ejecución.
    - Devuelve un ProcessingResult o lanza BackendError si algo falla.

Flujo de datos:
    MainWindow.validate_request()
        → ProcessingRequest
        → BackendRunner.run(request)
        → subprocess.run([ejecutable, --output, --filters, ...imágenes])
        → stdout: "TOTAL_TIME=0.003241\nOUTPUT_DIR=/ruta"
        → ProcessingResult(execution_time=0.003241, ...)
        → MainWindow.on_processing_finished(result)
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys


# ---------------------------------------------------------------------------
# Rutas al ejecutable C
# Se resuelven desde la ubicación de este archivo hacia la raíz del proyecto.
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent.parent
BACKEND_DIR = PROJECT_ROOT / "c_backend"
BACKEND_BIN_DIR = BACKEND_DIR / "bin"

# Ruta principal: c_backend/bin/para_image_parra(.exe en Windows)
BACKEND_EXECUTABLE = BACKEND_BIN_DIR / (
    "para_image_parra.exe" if sys.platform.startswith("win") else "para_image_parra"
)

# Fallback: c_backend/para_image_parra (si se compiló sin crear /bin)
BACKEND_EXECUTABLE_FALLBACK = BACKEND_DIR / (
    "para_image_parra.exe" if sys.platform.startswith("win") else "para_image_parra"
)

# Expresión regular para extraer el tiempo total del stdout del backend C.
# El backend imprime exactamente: TOTAL_TIME=0.003241
TIME_PATTERN = re.compile(r"TOTAL_TIME=([0-9]*\.?[0-9]+)")


# ---------------------------------------------------------------------------
# Excepciones
# ---------------------------------------------------------------------------

class BackendError(Exception):
    """
    Se lanza cuando el backend C termina con código de error distinto de 0,
    o cuando el stdout no contiene el campo TOTAL_TIME esperado.

    MainWindow captura esta excepción en on_processing_failed() y la
    muestra como QMessageBox.critical al usuario.
    """


# ---------------------------------------------------------------------------
# Dataclasses de entrada y salida
# ---------------------------------------------------------------------------

@dataclass(slots=True)
class ProcessingRequest:
    """
    Parámetros de una solicitud de procesamiento de imágenes.

    Construido en MainWindow.validate_request() a partir de los valores
    actuales de los widgets de la interfaz, y pasado a BackendRunner.run().

    Atributos:
        image_paths:  Lista de rutas absolutas a los archivos .bmp a procesar.
                      Origen: self.image_paths en MainWindow (lista maestra).
                      Ejemplo: ["/Users/sofia/foto1.bmp", "/Users/sofia/foto2.bmp"]

        output_dir:   Ruta de la carpeta donde se guardarán las imágenes procesadas.
                      Origen: output_dir_edit.text() en MainWindow.
                      Ejemplo: "/Users/sofia/proyecto/outputs"

        filters:      Lista de códigos de filtro seleccionados por el usuario.
                      Origen: selected_filters() → filter_checkboxes activos.
                      Valores posibles: "vg", "vc", "hg", "hc", "dg", "dc".
                      Ejemplo: ["vg", "hg", "dc"]

        kernel_gray:  Tamaño del kernel de desenfoque para el filtro "dg" (gris).
                      Origen: kernel_gray_spin.value() en MainWindow.
                      Solo se incluye si "dg" está en filters; si no, es None.
                      Debe ser entero positivo e impar (validado antes de crear este objeto).

        kernel_color: Tamaño del kernel de desenfoque para el filtro "dc" (color).
                      Origen: kernel_color_spin.value() en MainWindow.
                      Solo se incluye si "dc" está en filters; si no, es None.
                      Debe ser entero positivo e impar.

        executable:   Ruta al binario C compilado. Por defecto usa BACKEND_EXECUTABLE.
                      Se puede sobrescribir para pruebas o rutas personalizadas.
    """
    image_paths: list[str]
    output_dir: str
    filters: list[str]
    kernel_gray: int | None = None
    kernel_color: int | None = None
    executable: Path = BACKEND_EXECUTABLE


@dataclass(slots=True)
class ProcessingResult:
    """
    Resultado devuelto por BackendRunner.run() tras una ejecución exitosa.

    Construido a partir del stdout del proceso C y entregado a
    MainWindow.on_processing_finished() mediante la señal finished del worker.

    Atributos:
        execution_time: Tiempo total de procesamiento en segundos, extraído de
                        la línea "TOTAL_TIME=<valor>" del stdout del backend C.
                        Se muestra en execution_time_edit de la GUI.

        output_dir:     Carpeta donde quedaron guardadas las imágenes procesadas.
                        Extraída de "OUTPUT_DIR=<ruta>" del stdout del backend C.

        stdout:         Salida estándar completa del proceso C (para diagnóstico).

        stderr:         Salida de error del proceso C (útil para debugging).

        command:        Lista con el comando exacto que se ejecutó vía subprocess.
                        Útil para reproducir la ejecución manualmente en terminal.
    """
    execution_time: float
    output_dir: str
    stdout: str
    stderr: str
    command: list[str]


# ---------------------------------------------------------------------------
# Runner principal
# ---------------------------------------------------------------------------

class BackendRunner:
    """
    Traduce un ProcessingRequest en una llamada al ejecutable C y
    devuelve un ProcessingResult.

    Uso típico (desde ProcessingWorker en main_window.py):
        runner = BackendRunner()
        result = runner.run(request)
    """

    def __init__(self, executable: Path | None = None) -> None:
        """
        Args:
            executable: Ruta opcional al binario C. Si es None, usa
                        BACKEND_EXECUTABLE (c_backend/bin/para_image_parra).
        """
        self.executable = executable or BACKEND_EXECUTABLE

    def run(self, request: ProcessingRequest) -> ProcessingResult:
        """
        Ejecuta el backend C con los parámetros del request.

        Pasos internos:
            1. Verifica que el ejecutable C exista (prueba ruta principal y fallback).
            2. Crea la carpeta de salida si no existe.
            3. Construye la lista de argumentos CLI:
                   [ejecutable, --output, ruta, --filters, vg,dc,
                    --kernel-gray, N,  --kernel-color, M,
                    img1.bmp, img2.bmp, ...]
            4. Llama subprocess.run() con capture_output=True.
            5. Si returncode != 0, lanza BackendError con el mensaje de stderr.
            6. Busca TOTAL_TIME= en stdout con regex TIME_PATTERN.
            7. Devuelve ProcessingResult.

        Args:
            request: Objeto ProcessingRequest construido por MainWindow.

        Returns:
            ProcessingResult con el tiempo de ejecución y metadatos.

        Raises:
            BackendError: Si el ejecutable no existe, si el proceso falla,
                          o si stdout no contiene TOTAL_TIME.
        """
        # --- 1. Localizar el ejecutable ---
        executable = Path(request.executable or self.executable)
        if not executable.exists():
            executable = BACKEND_EXECUTABLE_FALLBACK
        if not executable.exists():
            raise BackendError(
                "No se encontro el ejecutable del backend. "
                "Compila primero c_backend/src/bmp_processor.c."
            )

        # --- 2. Crear carpeta de salida ---
        output_dir = Path(request.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # --- 3. Construir comando CLI ---
        # Argumentos obligatorios:
        command = [
            str(executable),
            "--output", str(output_dir),
            "--filters", ",".join(request.filters),  # p.ej. "vg,hg,dc"
        ]

        # Argumentos opcionales (solo si el filtro correspondiente está activo):
        if request.kernel_gray is not None:
            command.extend(["--kernel-gray", str(request.kernel_gray)])
        if request.kernel_color is not None:
            command.extend(["--kernel-color", str(request.kernel_color)])

        # Rutas de las imágenes (argumentos posicionales al final):
        command.extend(request.image_paths)

        # --- 4. Ejecutar el proceso C ---
        # En Windows: CREATE_NO_WINDOW evita que aparezca una consola cmd.
        creationflags = 0
        if sys.platform.startswith("win"):
            creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)

        completed = subprocess.run(
            command,
            cwd=PROJECT_ROOT,       # directorio de trabajo = raíz del proyecto
            capture_output=True,    # captura stdout y stderr
            text=True,
            encoding="utf-8",
            errors="replace",       # reemplaza bytes inválidos en lugar de fallar
            creationflags=creationflags,
            check=False,            # no lanza excepción en returncode != 0
        )

        # --- 5. Verificar código de retorno ---
        if completed.returncode != 0:
            error_text = (
                completed.stderr.strip()
                or completed.stdout.strip()
                or "El backend devolvio un error."
            )
            raise BackendError(error_text)

        # --- 6. Parsear TOTAL_TIME del stdout ---
        # El backend C imprime: TOTAL_TIME=0.003241
        match = TIME_PATTERN.search(completed.stdout)
        if not match:
            raise BackendError(
                "No se pudo leer el tiempo total desde la salida del backend."
            )

        # --- 7. Devolver resultado ---
        return ProcessingResult(
            execution_time=float(match.group(1)),
            output_dir=str(output_dir),
            stdout=completed.stdout,
            stderr=completed.stderr,
            command=command,
        )