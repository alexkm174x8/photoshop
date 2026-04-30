from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QDialog, QDialogButtonBox, QLabel, QVBoxLayout


class AboutDialog(QDialog):
    # aqui se construye la ventana pequena con la informacion del proyecto
    def __init__(self, parent=None) -> None:
        super().__init__(parent)

        # configuracion base del cuadro modal
        self.setObjectName("aboutDialog")
        self.setWindowTitle("Acerca de:")
        self.setModal(True)
        self.resize(420, 240)

        # encabezado principal
        title = QLabel("Acerca de")
        title.setObjectName("aboutTitle")

        # cuerpo informativo del proyecto y del equipo
        # qt.plaintext evita interpretar contenido como html
        body = QLabel(
            "TC3003B\n"
            "Tecnologico de Monterrey\n"
            "Campus Puebla\n"
            "Mayo 2026\n"
            "Equipo:\n"
            "Estefania Antonio Villaseca A01736897\n"
            "Miranda Eugenia Colorado Arroniz A01737023\n"
            "Alejandro Kong Montoya A01734271\n"
            "Sofia Zugasti Delgado A00837478"
        )
        body.setObjectName("aboutBody")
        body.setWordWrap(True)
        body.setTextFormat(Qt.PlainText)

        # boton estandar para cerrar el dialogo
        buttons = QDialogButtonBox(QDialogButtonBox.Ok)
        buttons.setObjectName("aboutButtons")
        buttons.accepted.connect(self.accept)

        # layout vertical:
        # titulo -> cuerpo -> separador flexible -> botones
        layout = QVBoxLayout(self)
        layout.addWidget(title)
        layout.addWidget(body)
        layout.addStretch(1)
        layout.addWidget(buttons)
