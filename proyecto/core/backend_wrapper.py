from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys


# rutas base del proyecto
# se usan para ubicar el backend en c sin depender de la carpeta
# desde donde el usuario ejecuta el programa
PROJECT_ROOT = Path(__file__).resolve().parent.parent
BACKEND_DIR = PROJECT_ROOT / "c_backend"
BACKEND_BIN_DIR = BACKEND_DIR / "bin"
BACKEND_EXECUTABLE = BACKEND_BIN_DIR / ("para_image_parra.exe" if sys.platform.startswith("win") else "para_image_parra")
BACKEND_EXECUTABLE_FALLBACK = BACKEND_DIR / ("para_image_parra.exe" if sys.platform.startswith("win") else "para_image_parra")

# el backend imprime en stdout una linea con el formato:
# total_time=<numero>
# esta expresion regular extrae ese numero para mostrarlo en la gui
TIME_PATTERN = re.compile(r"TOTAL_TIME=([0-9]*\.?[0-9]+)")


class BackendError(Exception):
    # excepcion especifica para errores de ejecucion o de contrato
    # entre gui y backend
    pass


@dataclass(slots=True)
class ProcessingRequest:
    # lista de rutas absolutas de imagenes bmp
    image_paths: list[str]
    # carpeta donde se guardaran resultados
    output_dir: str
    # codigos de filtros solicitados: vg, vc, hg, hc, dg, dc
    filters: list[str]
    # kernel para desenfoque gris (opcional, solo si se usa dg)
    kernel_gray: int | None = None
    # kernel para desenfoque color (opcional, solo si se usa dc)
    kernel_color: int | None = None
    # ruta de ejecutable concreta; se permite sobreescribir para pruebas
    executable: Path = BACKEND_EXECUTABLE


@dataclass(slots=True)
class ProcessingResult:
    # tiempo total que reporta el backend en segundos
    execution_time: float
    # carpeta efectiva de salida
    output_dir: str
    # salida estandar completa del backend
    stdout: str
    # salida de error completa del backend
    stderr: str
    # comando final que se ejecuto (util para depurar)
    command: list[str]


class BackendRunner:
    # aqui se guarda la ruta del ejecutable que se va a usar
    def __init__(self, executable: Path | None = None) -> None:
        # si no se recibe ruta, usa el ejecutable por defecto del proyecto
        self.executable = executable or BACKEND_EXECUTABLE

    # esta funcion arma el comando llama al exe y regresa el resultado para la gui
    def run(self, request: ProcessingRequest) -> ProcessingResult:
        # 1) resolver ejecutable
        # primero intenta la ruta declarada en request, luego un fallback
        executable = Path(request.executable or self.executable)
        if not executable.exists():
            executable = BACKEND_EXECUTABLE_FALLBACK
        if not executable.exists():
            raise BackendError(
                "No se encontro el ejecutable del backend. "
                "Compila primero c_backend/src/bmp_processor.c."
            )

        # 2) asegurar carpeta de salida antes de lanzar el proceso c
        output_dir = Path(request.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # 3) construir argumentos cli para el backend
        # el backend espera:
        #   --output <dir> --filters <lista> [kernels] <imagenes...>
        command = [
            str(executable),
            "--output",
            str(output_dir),
            "--filters",
            ",".join(request.filters),
        ]

        # kernels solo se envian si existen (si no, el backend no los usa)
        if request.kernel_gray is not None:
            command.extend(["--kernel-gray", str(request.kernel_gray)])
        if request.kernel_color is not None:
            command.extend(["--kernel-color", str(request.kernel_color)])

        # al final se agregan n rutas de imagen
        command.extend(request.image_paths)

        # 4) en windows ocultamos consola extra al correr el ejecutable
        creationflags = 0
        if sys.platform.startswith("win"):
            creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)

        # 5) ejecutar backend y capturar stdout/stderr
        # check=false para manejar nosotros los errores y devolver
        # mensajes claros al usuario
        completed = subprocess.run(
            command,
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=creationflags,
            check=False,
        )

        # 6) si el backend regresa error, priorizar stderr y propagar
        if completed.returncode != 0:
            error_text = completed.stderr.strip() or completed.stdout.strip() or "El backend devolvio un error."
            raise BackendError(error_text)

        # 7) parsear el tiempo total desde stdout para mostrarlo en gui
        match = TIME_PATTERN.search(completed.stdout)
        if not match:
            raise BackendError("No se pudo leer el tiempo total desde la salida del backend.")

        # 8) entregar estructura rica con datos utiles para la capa gui
        return ProcessingResult(
            execution_time=float(match.group(1)),
            output_dir=str(output_dir),
            stdout=completed.stdout,
            stderr=completed.stderr,
            command=command,
        )
