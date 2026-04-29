# BMP Parallel Studio

Aplicacion academica para procesamiento paralelo de imagenes BMP.

- Backend: C + OpenMP
- GUI: Python + PySide6

## 1. Objetivo

Permitir que el usuario cargue hasta 10 imagenes BMP, seleccione uno o varios filtros y procese el lote completo desde la interfaz.

## 2. Estructura del proyecto

- `gui/main.py`: arranque de la app y carga de estilos.
- `gui/main_window.py`: interfaz principal, validaciones y ejecucion.
- `gui/styles.qss`: apariencia visual.
- `core/backend_wrapper.py`: integracion Python -> ejecutable C.
- `c_backend/src/bmp_processor.c`: parser de argumentos y orquestacion.
- `c_backend/include/bmp_filters_core.h`: filtros `vg`, `vc`, `dc`.
- `c_backend/include/bmp_filters_extended.h`: filtros `hg`, `hc`, `dg` y utilidades.
- `outputs/`: carpeta de salida por defecto.
- `logo.webp`: logo mostrado en la GUI.

## 3. Requisitos

- Python 3.10+
- `pip`
- Compilador C con soporte OpenMP
- PySide6

Instalacion:

```bash
pip install PySide6
```

## 4. Compilar backend

Desde `proyecto/`:

Windows (MinGW):

```bash
mkdir c_backend\bin
gcc -fopenmp c_backend/src/bmp_processor.c -o c_backend/bin/para_image_parra.exe
```

Linux/macOS:

```bash
mkdir -p c_backend/bin
gcc -fopenmp c_backend/src/bmp_processor.c -o c_backend/bin/para_image_parra
```

## 5. Ejecutar GUI

Desde `proyecto/`:

```bash
python gui/main.py
```

## 6. Flujo UI -> Proceso

1. Usuario agrega BMP por drag and drop o selector.
2. GUI valida extension, duplicados y limite de 10.
3. Usuario selecciona filtros y kernels (si aplica desenfoque).
4. Usuario define carpeta de salida.
5. Al presionar `Ejecutar`, la GUI construye `ProcessingRequest`.
6. `BackendRunner` ejecuta el backend C con:
   - `--output`
   - `--filters`
   - `--kernel-gray` (si `dg`)
   - `--kernel-color` (si `dc`)
   - lista de imagenes
7. Backend procesa y crea archivos de salida.
8. GUI muestra tiempo total y estado final.

## 7. Convencion de nombres de salida

Formato:

`<nombre_original>_<acronimo>.bmp`

Ejemplos:

- `Fotoa.bmp` -> `Fotoa_vg.bmp`
- `Fotoa.bmp` -> `Fotoa_vc.bmp`
- `Fotoa.bmp` -> `Fotoa_hg.bmp`
- `Fotoa.bmp` -> `Fotoa_hc.bmp`
- `Fotoa.bmp` -> `Fotoa_dg.bmp`
- `Fotoa.bmp` -> `Fotoa_dc.bmp`

## 8. Seis transformaciones implementadas

- `vg`: inversion vertical en escala de grises (`inv_img`)
- `vc`: inversion vertical a color (`inv_img_color`)
- `hg`: espejo horizontal en escala de grises (`inv_img_grey_horizontal`)
- `hc`: espejo horizontal a color (`inv_img_color_horizontal`)
- `dg`: desenfoque en escala de grises (`desenfoque`)
- `dc`: desenfoque a color (`desenfoque_color`)

## 9. Documentacion de elementos graficos y acciones

Esta seccion puede copiarse tal cual a la wiki.

### 9.1 Zona de carga

- Elemento: `DropArea`
- Evento: `dropEvent`
- Accion detonada: emite `files_dropped`
- Proceso asociado: `add_images(paths)` valida BMP, elimina duplicados y respeta limite.

### 9.2 Botones de imagenes

- Boton: `Agregar BMP`
- Accion detonada: `select_images()`
- Proceso asociado: abre selector y llama `add_images(files)`.

- Boton: `Quitar seleccion`
- Accion detonada: `remove_selected_images()`
- Proceso asociado: elimina del modelo interno las rutas seleccionadas.

- Boton: `Limpiar lista`
- Accion detonada: `clear_images()`
- Proceso asociado: vacia lista y contador.

### 9.3 Lista y contador

- Elemento: `QListWidget image_list`
- Accion detonada: seleccion multiple para eliminacion.
- Proceso asociado: muestra nombre y ruta de cada BMP.

- Elemento: `loaded_count_label`
- Accion detonada: actualizacion en `refresh_image_list()`.
- Proceso asociado: muestra `N / 10 imagenes cargadas`.

### 9.4 Bloque de transformaciones

- Elemento: `QCheckBox "Todas"`
- Accion detonada: `toggle_all_filters(checked)`
- Proceso asociado: marca/desmarca todos los filtros.

- Elementos: `vg`, `vc`, `hg`, `hc`, `dg`, `dc`
- Accion detonada: cambios individuales de seleccion.
- Proceso asociado: `selected_filters()` define los filtros enviados al backend.

### 9.5 Bloque de kernels

- Elemento: `kernel_gray_spin`
- Accion detonada: valor para `dg`.
- Proceso asociado: validacion de entero positivo impar.

- Elemento: `kernel_color_spin`
- Accion detonada: valor para `dc`.
- Proceso asociado: validacion de entero positivo impar.

### 9.6 Salida y tiempo

- Elemento: `output_dir_edit`
- Accion detonada: refleja la ruta de salida activa.

- Boton: `Elegir carpeta`
- Accion detonada: `select_output_dir()`
- Proceso asociado: seleccion de carpeta con `QFileDialog`.

- Elemento: `execution_time_edit`
- Accion detonada: actualizacion al terminar.
- Proceso asociado: muestra `TOTAL_TIME` reportado por backend.

### 9.7 Ejecucion

- Boton: `Ejecutar`
- Accion detonada: `start_processing()`
- Proceso asociado:
  - valida solicitud
  - inicia `ProcessingWorker` en `QThread`
  - llama backend C
  - bloquea boton durante ejecucion
  - muestra resultado o error

### 9.8 Ayuda y branding

- Menu/Boton: `Acerca de`
- Accion detonada: `show_about_dialog()`
- Proceso asociado: muestra informacion general del proyecto.

- Elemento: `logoLabel` (imagen `logo.webp`)
- Accion detonada: carga visual al construir panel derecho.
- Proceso asociado: branding de la interfaz.

## 10. Validaciones implementadas

- Solo se aceptan archivos `.bmp`.
- Maximo 10 imagenes por ejecucion.
- Al menos una imagen cargada.
- Al menos un filtro seleccionado.
- Kernel impar y positivo para `dg`/`dc`.
- Carpeta de salida valida.

## 11. Conexion tecnica entre GUI y backend

- Clase GUI: construye `ProcessingRequest`.
- Wrapper: arma linea de comandos y ejecuta binario.
- Backend C: procesa y responde por `stdout`:
  - `TOTAL_TIME=<valor>`
  - `OUTPUT_DIR=<ruta>`
- GUI: parsea tiempo y actualiza interfaz.

## 12. Nota para wiki de la materia

Para cumplir la consigna de documentacion, usar como seccion de wiki:

- Seccion 6 (Flujo UI -> Proceso)
- Seccion 9 (Elementos graficos y acciones)
- Seccion 11 (Conexion tecnica GUI/backend)
