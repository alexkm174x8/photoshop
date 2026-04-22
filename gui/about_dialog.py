from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QDialog, QDialogButtonBox, QLabel, QVBoxLayout


class AboutDialog(QDialog):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Acerca de")
        self.setModal(True)
        self.resize(420, 240)

        title = QLabel("Procesamiento de Imagenes BMP")
        title.setObjectName("aboutTitle")

        body = QLabel(
            "Aplicacion de apoyo para un proyecto de procesamiento paralelo de imagenes BMP.\n\n"
            "Curso: Programacion Paralela / Procesamiento de Imagenes.\n"
            "Tecnologias: C con OpenMP para el backend y Python con PySide6 para la interfaz.\n"
            "Equipo: proyecto academico de procesamiento BMP con interfaz grafica reutilizable."
        )
        body.setWordWrap(True)
        body.setTextFormat(Qt.PlainText)

        buttons = QDialogButtonBox(QDialogButtonBox.Ok)
        buttons.accepted.connect(self.accept)

        layout = QVBoxLayout(self)
        layout.addWidget(title)
        layout.addWidget(body)
        layout.addStretch(1)
        layout.addWidget(buttons)
