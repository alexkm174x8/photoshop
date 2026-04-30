from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from PySide6.QtCore import QObject, QThread, Qt, Signal
from PySide6.QtGui import QAction, QPixmap
from PySide6.QtWidgets import (
    QAbstractItemView,
    QApplication,
    QCheckBox,
    QFileDialog,
    QFormLayout,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from core.backend_wrapper import BACKEND_DIR, BackendError, BackendRunner, ProcessingRequest, ProcessingResult
from gui.about_dialog import AboutDialog


# rutas globales usadas por la gui
PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT_DIR = PROJECT_ROOT / "outputs"

# restriccion pedida por el proyecto: maximo 10 imagenes por ejecucion
MAX_IMAGES = 10

# mapeo: codigo del filtro en backend -> etiqueta visible en interfaz
FILTER_LABELS = {
    "vg": "Inversion vertical gris",
    "vc": "Inversion vertical color",
    "hg": "Espejo horizontal gris",
    "hc": "Espejo horizontal color",
    "dg": "Desenfoque gris",
    "dc": "Desenfoque color",
}


class DropArea(QFrame):
    # senal custom para notificar a la ventana principal la lista de rutas soltadas
    files_dropped = Signal(list)

    # aqui se prepara el area donde se sueltan archivos con el mouse
    def __init__(self, parent=None) -> None:
        super().__init__(parent)

        # habilita drag and drop solo para este contenedor
        self.setAcceptDrops(True)
        self.setObjectName("dropArea")

        # mensaje guia para usuario
        label = QLabel("Arrastra hasta 10 imagenes BMP aqui\n\no usa el boton Agregar BMP")
        label.setAlignment(Qt.AlignCenter)
        label.setWordWrap(True)

        layout = QVBoxLayout(self)
        layout.addStretch(1)
        layout.addWidget(label)
        layout.addStretch(1)

    # este evento decide si el archivo que entra se puede aceptar
    def dragEnterEvent(self, event) -> None:  # noqa: N802
        # acepta el drag solo si contiene urls (archivos)
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
        else:
            event.ignore()

    # este evento mantiene activo el permiso mientras se mueve el archivo
    def dragMoveEvent(self, event) -> None:  # noqa: N802
        # misma validacion durante el movimiento dentro de la zona
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
        else:
            event.ignore()

    # aqui se toman las rutas soltadas y se mandan a la ventana principal
    def dropEvent(self, event) -> None:  # noqa: N802
        # convierte urls a rutas locales y emite una sola senal con lista
        paths = [url.toLocalFile() for url in event.mimeData().urls() if url.isLocalFile()]
        self.files_dropped.emit(paths)
        event.acceptProposedAction()


class ProcessingWorker(QObject):
    # finished envia processingresult en object para no acoplar tipos qt/python
    finished = Signal(object)
    failed = Signal(str)

    def __init__(self, request: ProcessingRequest) -> None:
        super().__init__()
        self.request = request
        self.runner = BackendRunner()

    # esta funcion corre en segundo plano y llama al backend en c
    def run(self) -> None:
        # metodo ejecutado en hilo secundario
        # mantiene la gui responsiva mientras corre el backend en c
        try:
            result = self.runner.run(self.request)
        except BackendError as exc:
            self.failed.emit(str(exc))
            return
        except Exception as exc:  # pragma: no cover
            self.failed.emit(f"Error inesperado: {exc}")
            return

        self.finished.emit(result)


@dataclass(slots=True)
class ValidationResult:
    # request sera none cuando falle una validacion
    request: ProcessingRequest | None
    # mensaje para usuario en caso de error de validacion
    error: str | None = None


class MainWindow(QMainWindow):
    # aqui se crea la ventana principal y se inicializa el estado base
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("BMP Parallel Studio")
        self.resize(1080, 680)

        # referencias para manejar ciclo de vida del hilo de procesamiento
        self._thread: QThread | None = None
        self._worker: ProcessingWorker | None = None

        # estado principal de gui:
        # - image_paths: archivos seleccionados por usuario
        # - filter_checkboxes: accesos a checks por codigo de filtro
        self.image_paths: list[str] = []
        self.filter_checkboxes: dict[str, QCheckBox] = {}

        # garantiza que exista la carpeta de salida default
        DEFAULT_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

        self._build_menu()
        self._build_ui()
        self.statusBar().showMessage(f"Backend esperado en: {BACKEND_DIR}")

    # aqui se arma el menu superior y se conecta el boton acerca de
    def _build_menu(self) -> None:
        # menu minimo: opcion "acerca de"
        about_action = QAction("Acerca de", self)
        about_action.triggered.connect(self.show_about_dialog)
        self.menuBar().addMenu("Ayuda").addAction(about_action)

    # aqui se construye la pantalla completa con panel izquierdo y derecho
    def _build_ui(self) -> None:
        # layout principal con 2 columnas:
        # izquierda = carga de imagenes
        # derecha = opciones de procesamiento
        container = QWidget()
        root = QHBoxLayout(container)
        root.setContentsMargins(20, 20, 20, 20)
        root.setSpacing(18)

        left_container = QWidget()
        left_layout = QVBoxLayout(left_container)
        left_layout.setSpacing(10)
        left_layout.setContentsMargins(0, 0, 0, 0)

        # cabecera izquierda con logo institucional y boton "acerca de"
        logo_label = QLabel()
        logo_path = PROJECT_ROOT / "logo_tec.png"
        pixmap = QPixmap(str(logo_path))

        logo_label.setPixmap(pixmap.scaledToWidth(220, Qt.SmoothTransformation))
        logo_label.setAlignment(Qt.AlignLeft)

        about_button = QPushButton("Acerca de")
        about_button.clicked.connect(self.show_about_dialog)

        logo_row = QHBoxLayout()
        logo_row.addWidget(logo_label)
        logo_row.addStretch(1)  # empuja el boton hacia la derecha
        logo_row.addWidget(about_button)

        left_panel = self._build_left_panel()

        left_layout.addLayout(logo_row)
        left_layout.addWidget(left_panel)

        # proporcion 5:4 entre panel izquierdo y derecho
        root.addWidget(left_container, 5)
        root.addWidget(self._build_right_panel(), 4)

        self.setCentralWidget(container)

    # aqui se arma la parte de carga de imagenes y lista visual
    def _build_left_panel(self) -> QWidget:
        # panel de entrada de archivos
        panel = QFrame()
        panel.setObjectName("card")
        layout = QVBoxLayout(panel)
        layout.setSpacing(14)

        title = QLabel("Imagenes BMP")
        title.setObjectName("sectionTitle")

        subtitle = QLabel("Carga archivos BMP con arrastrar y soltar o con el selector.")
        subtitle.setObjectName("mutedLabel")

        # area de drag&drop
        self.drop_area = DropArea()
        self.drop_area.files_dropped.connect(self.add_images)

        # botones de gestion de lista
        buttons = QHBoxLayout()
        add_button = QPushButton("Agregar BMP")
        add_button.clicked.connect(self.select_images)

        remove_button = QPushButton("Quitar seleccion")
        remove_button.clicked.connect(self.remove_selected_images)

        clear_button = QPushButton("Limpiar lista")
        clear_button.clicked.connect(self.clear_images)

        buttons.addWidget(add_button)
        buttons.addWidget(remove_button)
        buttons.addWidget(clear_button)

        # lista visual de imagenes cargadas
        self.image_list = QListWidget()
        self.image_list.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.image_list.setAlternatingRowColors(True)

        # indicador del limite maximo
        self.loaded_count_label = QLabel("0 / 10 imagenes cargadas")
        self.loaded_count_label.setObjectName("mutedLabel")

        layout.addWidget(title)
        layout.addWidget(subtitle)
        layout.addWidget(self.drop_area, 4)
        layout.addLayout(buttons)
        layout.addWidget(self.image_list, 5)
        layout.addWidget(self.loaded_count_label)
        return panel

    # aqui se arma la parte de filtros kernels salida y boton ejecutar
    def _build_right_panel(self) -> QWidget:
        # panel de configuracion de filtros, kernels y salida
        panel = QFrame()
        panel.setObjectName("card")
        layout = QVBoxLayout(panel)
        layout.setSpacing(16)

        title = QLabel("Opciones de procesamiento")
        title.setObjectName("sectionTitle")

        # grupo de checkboxes de transformaciones
        transform_group = QGroupBox("Transformaciones")
        transform_layout = QGridLayout(transform_group)

        # checkbox maestro para seleccionar/deseleccionar todos
        self.all_checkbox = QCheckBox("Todas")
        self.all_checkbox.toggled.connect(self.toggle_all_filters)
        transform_layout.addWidget(self.all_checkbox, 0, 0, 1, 2)

        # checkboxes individuales por filtro
        for index, (filter_code, label) in enumerate(FILTER_LABELS.items(), start=1):
            checkbox = QCheckBox(label)
            checkbox.toggled.connect(self.sync_all_checkbox)
            self.filter_checkboxes[filter_code] = checkbox
            row = (index + 1) // 2
            column = (index - 1) % 2
            transform_layout.addWidget(checkbox, row, column)

        # grupo para tamanos de kernel de desenfoque
        kernel_group = QGroupBox("Kernels de desenfoque")
        kernel_layout = QFormLayout(kernel_group)

        self.kernel_gray_spin = self._create_kernel_spinbox()
        self.kernel_color_spin = self._create_kernel_spinbox()
        kernel_layout.addRow("Kernel gris:", self.kernel_gray_spin)
        kernel_layout.addRow("Kernel color:", self.kernel_color_spin)

        # grupo de ruta de salida y campo de tiempo total
        output_group = QGroupBox("Salida y resultado")
        output_layout = QFormLayout(output_group)

        output_row = QHBoxLayout()
        self.output_dir_edit = QLineEdit(str(DEFAULT_OUTPUT_DIR))
        self.output_dir_edit.setReadOnly(True)
        browse_output_button = QPushButton("Elegir carpeta")
        browse_output_button.clicked.connect(self.select_output_dir)
        output_row.addWidget(self.output_dir_edit, 1)
        output_row.addWidget(browse_output_button)

        self.execution_time_edit = QLineEdit()
        self.execution_time_edit.setReadOnly(True)
        self.execution_time_edit.setPlaceholderText("Aun sin ejecutar")

        output_layout.addRow("Ruta de salida:", self._wrap_layout(output_row))
        output_layout.addRow("Tiempo total:", self.execution_time_edit)

        # boton principal de accion
        action_row = QHBoxLayout()
        self.execute_button = QPushButton("Ejecutar")
        self.execute_button.setObjectName("primaryButton")
        self.execute_button.clicked.connect(self.start_processing)

        action_row.addWidget(self.execute_button, 1)

        backend_hint = QLabel("El boton Ejecutar se bloquea mientras corre el backend en C.")
        backend_hint.setObjectName("mutedLabel")

        layout.addWidget(title)
        layout.addWidget(transform_group)
        layout.addWidget(kernel_group)
        layout.addWidget(output_group)
        layout.addWidget(backend_hint)
        layout.addStretch(1)
        layout.addLayout(action_row)
        return panel

    # esta funcion prepara el control donde el usuario cambia el kernel
    def _create_kernel_spinbox(self) -> QSpinBox:
        # se fuerza rango positivo y paso impar para facilitar validacion
        spin = QSpinBox()
        spin.setRange(1, 999)
        spin.setSingleStep(2)
        spin.setValue(27)
        return spin

    def _wrap_layout(self, layout) -> QWidget:
        # utilidad para insertar un layout dentro de un qformlayout
        wrapper = QWidget()
        wrapper.setLayout(layout)
        return wrapper

    # esta funcion abre el explorador para elegir imagenes bmp
    def select_images(self) -> None:
        # selector de archivos bmp
        files, _ = QFileDialog.getOpenFileNames(
            self,
            "Seleccionar imagenes BMP",
            str(PROJECT_ROOT),
            "Imagenes BMP (*.bmp)",
        )
        if files:
            self.add_images(files)

    # aqui se filtran las rutas se valida bmp y se actualiza la lista
    def add_images(self, paths: list[str]) -> None:
        # 1) normaliza rutas a absolutas
        normalized_paths = [str(Path(path).resolve()) for path in paths if path]
        # 2) filtra solo archivos con extension .bmp
        bmp_paths = [path for path in normalized_paths if Path(path).suffix.lower() == ".bmp"]

        if len(bmp_paths) != len(normalized_paths):
            QMessageBox.warning(self, "Archivos no validos", "Solo se aceptan archivos con extension .bmp.")
            return

        # evita duplicados en el estado interno
        unique_new_paths = [path for path in bmp_paths if path not in self.image_paths]
        if len(self.image_paths) + len(unique_new_paths) > MAX_IMAGES:
            QMessageBox.warning(self, "Limite excedido", "Solo puedes cargar un maximo de 10 imagenes BMP.")
            return

        self.image_paths.extend(unique_new_paths)
        self.refresh_image_list()

    # esta funcion quita solo los elementos que el usuario selecciono
    def remove_selected_images(self) -> None:
        # elimina solo los elementos seleccionados en qlistwidget
        selected_items = self.image_list.selectedItems()
        if not selected_items:
            return

        selected_paths = {item.data(Qt.UserRole) for item in selected_items}
        self.image_paths = [path for path in self.image_paths if path not in selected_paths]
        self.refresh_image_list()

    # esta funcion borra toda la lista de imagenes cargadas
    def clear_images(self) -> None:
        # limpieza total de seleccion
        self.image_paths.clear()
        self.refresh_image_list()

    # aqui se redibuja la lista y el contador de imagenes
    def refresh_image_list(self) -> None:
        # refresca representacion visual de la lista interna de rutas
        self.image_list.clear()
        for path in self.image_paths:
            item = QListWidgetItem(f"{Path(path).name}\n{path}")
            # userrole guarda la ruta real para operaciones posteriores
            item.setData(Qt.UserRole, path)
            self.image_list.addItem(item)

        self.loaded_count_label.setText(f"{len(self.image_paths)} / {MAX_IMAGES} imagenes cargadas")

    # esta funcion deja elegir la carpeta donde se guardan resultados
    def select_output_dir(self) -> None:
        # selector de carpeta de salida
        directory = QFileDialog.getExistingDirectory(
            self,
            "Seleccionar carpeta de salida",
            self.output_dir_edit.text() or str(DEFAULT_OUTPUT_DIR),
        )
        if directory:
            self.output_dir_edit.setText(directory)

    # esta funcion marca o desmarca todos los filtros con un solo check
    def toggle_all_filters(self, checked: bool) -> None:
        # activa/desactiva todos los filtros en bloque
        # blocksignals evita bucles con sync_all_checkbox
        for checkbox in self.filter_checkboxes.values():
            checkbox.blockSignals(True)
            checkbox.setChecked(checked)
            checkbox.blockSignals(False)

    # esta funcion mantiene sincronizado el check de todas
    def sync_all_checkbox(self) -> None:
        # si todos los filtros individuales estan activos, marca "todas"
        all_selected = all(checkbox.isChecked() for checkbox in self.filter_checkboxes.values())
        self.all_checkbox.blockSignals(True)
        self.all_checkbox.setChecked(all_selected)
        self.all_checkbox.blockSignals(False)

    # esta funcion toma los checks activos y arma la lista de codigos
    def selected_filters(self) -> list[str]:
        # convierte estado de checkboxes a codigos del backend
        return [code for code, checkbox in self.filter_checkboxes.items() if checkbox.isChecked()]

    # aqui se revisa que todo este bien antes de mandar al backend
    def validate_request(self) -> ValidationResult:
        # validaciones previas a correr backend
        # devuelve validationresult para separar validacion de ui modal
        if not self.image_paths:
            return ValidationResult(None, "Carga al menos una imagen BMP antes de ejecutar.")

        if len(self.image_paths) > MAX_IMAGES:
            return ValidationResult(None, "El maximo permitido es de 10 imagenes BMP.")

        selected_filters = self.selected_filters()
        if not selected_filters:
            return ValidationResult(None, "Selecciona al menos una transformacion.")

        kernel_gray = self.kernel_gray_spin.value()
        kernel_color = self.kernel_color_spin.value()

        if "dg" in selected_filters and not self._is_valid_kernel(kernel_gray):
            return ValidationResult(None, "El kernel para desenfoque gris debe ser un entero positivo e impar.")

        if "dc" in selected_filters and not self._is_valid_kernel(kernel_color):
            return ValidationResult(None, "El kernel para desenfoque color debe ser un entero positivo e impar.")

        output_dir = self.output_dir_edit.text().strip()
        if not output_dir:
            return ValidationResult(None, "Selecciona una carpeta de salida.")

        # construye la solicitud final para el wrapper del backend
        request = ProcessingRequest(
            image_paths=list(self.image_paths),
            output_dir=output_dir,
            filters=selected_filters,
            kernel_gray=kernel_gray if "dg" in selected_filters else None,
            kernel_color=kernel_color if "dc" in selected_filters else None,
        )
        return ValidationResult(request)

    # esta regla valida que el kernel sea positivo e impar
    def _is_valid_kernel(self, value: int) -> bool:
        # regla de kernel: entero positivo e impar
        return value > 0 and value % 2 == 1

    # esta funcion es el punto principal cuando se presiona ejecutar
    def start_processing(self) -> None:
        # metodo invocado por el boton "ejecutar"
        validation = self.validate_request()
        if validation.error:
            QMessageBox.warning(self, "Validacion", validation.error)
            return

        request = validation.request
        if request is None:
            QMessageBox.warning(self, "Validacion", "No se pudo construir la solicitud.")
            return

        # bloquea boton para prevenir ejecuciones concurrentes
        self.execute_button.setEnabled(False)
        self.execution_time_edit.setText("Procesando...")
        self.statusBar().showMessage("Ejecutando backend...")

        # configura worker en hilo separado para no congelar la interfaz
        self._thread = QThread(self)
        self._worker = ProcessingWorker(request)
        self._worker.moveToThread(self._thread)
        self._thread.started.connect(self._worker.run)
        self._worker.finished.connect(self.on_processing_finished)
        self._worker.failed.connect(self.on_processing_failed)
        self._worker.finished.connect(self._thread.quit)
        self._worker.failed.connect(self._thread.quit)
        self._thread.finished.connect(self._cleanup_thread)
        self._thread.start()

    # aqui llega la respuesta exitosa y se muestra tiempo y resumen
    def on_processing_finished(self, result: ProcessingResult) -> None:
        # ruta de exito
        self.execute_button.setEnabled(True)
        self.execution_time_edit.setText(f"{result.execution_time:.3f} s")
        self.output_dir_edit.setText(result.output_dir)
        self.statusBar().showMessage("Procesamiento finalizado.")

        # estimacion de cuantas salidas debieron generarse
        total_outputs = len(self.image_paths) * len(self.selected_filters())
        QMessageBox.information(
            self,
            "Proceso completado",
            "Las imagenes se procesaron correctamente.\n\n"
            f"Tiempo total: {result.execution_time:.3f} s\n"
            f"Ruta de salida: {result.output_dir}\n"
            f"Archivos esperados: {total_outputs}",
        )

    # aqui llega cualquier error y se muestra el mensaje al usuario
    def on_processing_failed(self, error_message: str) -> None:
        # ruta de error
        self.execute_button.setEnabled(True)
        self.execution_time_edit.clear()
        self.statusBar().showMessage("Fallo la ejecucion del backend.")
        QMessageBox.critical(self, "Error de procesamiento", error_message)

    # esta funcion limpia objetos del hilo cuando termina el proceso
    def _cleanup_thread(self) -> None:
        # libera objetos qt asociados al hilo al terminar
        if self._worker is not None:
            self._worker.deleteLater()
            self._worker = None

        if self._thread is not None:
            self._thread.deleteLater()
            self._thread = None

    # esta funcion abre la ventana con los datos generales del proyecto
    def show_about_dialog(self) -> None:
        # muestra el dialogo modal "acerca de"
        dialog = AboutDialog(self)
        dialog.exec()


if __name__ == "__main__":
    # permite ejecutar este archivo por separado para pruebas manuales
    app = QApplication([])
    window = MainWindow()
    window.show()
    app.exec()
