from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parent.parent
BACKEND_DIR = PROJECT_ROOT / "c_backend"
BACKEND_BIN_DIR = BACKEND_DIR / "bin"
BACKEND_EXECUTABLE = BACKEND_BIN_DIR / ("para_image_parra.exe" if sys.platform.startswith("win") else "para_image_parra")
BACKEND_EXECUTABLE_FALLBACK = BACKEND_DIR / ("para_image_parra.exe" if sys.platform.startswith("win") else "para_image_parra")
TIME_PATTERN = re.compile(r"TOTAL_TIME=([0-9]*\.?[0-9]+)")


class BackendError(Exception):
    pass


@dataclass(slots=True)
class ProcessingRequest:
    image_paths: list[str]
    output_dir: str
    filters: list[str]
    kernel_gray: int | None = None
    kernel_color: int | None = None
    executable: Path = BACKEND_EXECUTABLE


@dataclass(slots=True)
class ProcessingResult:
    execution_time: float
    output_dir: str
    stdout: str
    stderr: str
    command: list[str]


class BackendRunner:
    def __init__(self, executable: Path | None = None) -> None:
        self.executable = executable or BACKEND_EXECUTABLE

    def run(self, request: ProcessingRequest) -> ProcessingResult:
        executable = Path(request.executable or self.executable)
        if not executable.exists():
            executable = BACKEND_EXECUTABLE_FALLBACK
        if not executable.exists():
            raise BackendError(
                "No se encontro el ejecutable del backend. "
                "Compila primero c_backend/src/bmp_processor.c."
            )

        output_dir = Path(request.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        command = [
            str(executable),
            "--output",
            str(output_dir),
            "--filters",
            ",".join(request.filters),
        ]

        if request.kernel_gray is not None:
            command.extend(["--kernel-gray", str(request.kernel_gray)])
        if request.kernel_color is not None:
            command.extend(["--kernel-color", str(request.kernel_color)])

        command.extend(request.image_paths)

        creationflags = 0
        if sys.platform.startswith("win"):
            creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)

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

        if completed.returncode != 0:
            error_text = completed.stderr.strip() or completed.stdout.strip() or "El backend devolvio un error."
            raise BackendError(error_text)

        match = TIME_PATTERN.search(completed.stdout)
        if not match:
            raise BackendError("No se pudo leer el tiempo total desde la salida del backend.")

        return ProcessingResult(
            execution_time=float(match.group(1)),
            output_dir=str(output_dir),
            stdout=completed.stdout,
            stderr=completed.stderr,
            command=command,
        )
