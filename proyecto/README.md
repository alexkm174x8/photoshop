# Proyecto: Procesamiento paralelo de imagenes BMP

Este proyecto combina:

- **Backend en C + OpenMP** para aplicar filtros a imagenes BMP.
- **Interfaz grafica en Python (PySide6)** para seleccionar imagenes y ejecutar el backend.

---

## 1) Requisitos

- Python 3.10 o superior
- `pip`
- Compilador C con soporte OpenMP (`gcc` recomendado)

Instalar dependencia de Python:

```bash
pip install PySide6
```

---

## 2) Estructura del proyecto

- `gui/`: interfaz grafica (ventana principal, estilos y dialogos)
- `core/`: logica de integracion entre GUI y backend
- `c_backend/src/`: programa principal en C (`bmp_processor.c`)
- `c_backend/include/`: funciones de filtros BMP
- `outputs/`: carpeta de salida por defecto para imagenes procesadas

---

## 3) Compilar el backend

Desde la carpeta `proyecto/`:

### Linux / macOS

```bash
mkdir -p c_backend/bin
gcc -fopenmp c_backend/src/bmp_processor.c -o c_backend/bin/para_image_parra
```

### Windows (MinGW)

```bash
mkdir c_backend\bin
gcc -fopenmp c_backend/src/bmp_processor.c -o c_backend/bin/para_image_parra.exe
```

> Nota: la app busca primero el ejecutable en `c_backend/bin/` y, como respaldo, en `c_backend/`.

---

## 4) Ejecutar la interfaz

Desde `proyecto/`:

```bash
python gui/main.py
```

Flujo rapido:

1. Agregar imagenes `.bmp` (maximo 10).
2. Elegir filtros.
3. (Opcional) ajustar kernel para desenfoque (impar y positivo).
4. Elegir carpeta de salida.
5. Presionar **Ejecutar**.

---

## 5) Como funciona el codigo

## GUI (`gui/`)

- `main.py` inicia la aplicacion Qt y carga estilos desde `styles.qss`.
- `main_window.py` construye la ventana principal:
  - carga imagenes BMP
  - seleccion de filtros
  - validaciones de entrada
  - ejecucion en hilo separado para no congelar la UI

## Integracion (`core/backend_wrapper.py`)

- Construye el comando para el ejecutable en C.
- Pasa argumentos como:
  - `--output`
  - `--filters`
  - `--kernel-gray`
  - `--kernel-color`
- Ejecuta el proceso y captura `stdout`/`stderr`.
- Lee el tiempo total desde la salida del backend (`TOTAL_TIME=...`).

## Backend C (`c_backend/src/bmp_processor.c`)

- Parsea argumentos de linea de comandos.
- Valida filtros, kernels e imagenes BMP.
- Aplica filtros en paralelo usando secciones OpenMP.
- Genera archivos de salida en la carpeta indicada.
- Imprime:
  - `TOTAL_TIME=<segundos>`
  - `OUTPUT_DIR=<ruta>`

Los filtros concretos estan implementados en:

- `c_backend/include/bmp_filters_core.h`
- `c_backend/include/bmp_filters_extended.h`

---

## 6) Problemas comunes

- **"No se encontro el ejecutable del backend"**  
  Compilar de nuevo el backend (seccion 3).

- **Error con OpenMP al compilar**  
  Verificar que tu compilador soporte `-fopenmp`.

- **No procesa imagenes**  
  Confirmar que los archivos sean `.bmp` validos.
