from __future__ import annotations

from pathlib import Path
import sys

from PySide6.QtWidgets import QApplication


# carpeta de este archivo (gui/) y raiz del proyecto
CURRENT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = CURRENT_DIR.parent

# asegura que python pueda importar paquetes del proyecto aunque
# la app se ejecute desde otra ruta
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from gui.main_window import MainWindow


# esta funcion carga el estilo visual de toda la aplicacion
def load_stylesheet(app: QApplication) -> None:
    # carga estilos visuales globales de qt (si existe el archivo)
    stylesheet_path = CURRENT_DIR / "styles.qss"
    if stylesheet_path.exists():
        app.setStyleSheet(stylesheet_path.read_text(encoding="utf-8"))


# aqui arranca toda la aplicacion de escritorio
def main() -> int:
    # punto de entrada de gui:
    # 1) crear aplicacion
    # 2) aplicar estilo
    # 3) mostrar ventana principal
    # 4) entrar al event loop
    app = QApplication(sys.argv)
    load_stylesheet(app)
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    # systemexit permite regresar codigo de salida al sistema operativo
    raise SystemExit(main())
