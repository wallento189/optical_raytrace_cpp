#!/usr/bin/env python3
"""Cross-platform release builder for optical_raytrace_ui.

Usage:  python3 build_release.py

Builds the C++ engine via CMake, then packages everything into a single
executable via PyInstaller. Works on Windows, macOS, and Linux.

Prerequisites:
  - C++17 compiler (g++, clang++, or MSVC)
  - CMake >= 3.16
  - Python 3 + pip (flask, pyinstaller)
"""

import os
import sys
import shutil
import subprocess
import platform
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DIST = ROOT / "dist"
BUILD_DIR = ROOT / "build"
PI_BUILD_DIR = ROOT / "build_pyinstaller"

IS_WIN = sys.platform == "win32"
IS_MAC = sys.platform == "darwin"
BIN_NAME = "optical_raytrace.exe" if IS_WIN else "optical_raytrace"
SPEC_FILE = ROOT / "optical_raytrace.spec"


def cmd(args, **kw):
    """Print and run a command; raise on failure."""
    print(f"  > {' '.join(str(a) for a in args)}")
    subprocess.run(args, check=True, **kw)


def find_cmake_generator():
    """Pick a suitable CMake generator for the current platform."""
    if not IS_WIN:
        return None  # default (Unix Makefiles) is fine

    # Try Visual Studio first, fall back to Ninja / MinGW
    result = subprocess.run(["cmake", "--help"], capture_output=True, text=True)
    for name in ("Visual Studio 17 2022", "Visual Studio 16 2019",
                 "MinGW Makefiles", "Ninja"):
        if name in result.stdout:
            return name
    return None


def install_python_deps():
    """Ensure flask and pyinstaller are available."""
    for pkg in ("flask", "pyinstaller"):
        try:
            __import__(pkg.replace("-", "_"))
        except ImportError:
            print(f"Installing {pkg} ...")
            cmd([sys.executable, "-m", "pip", "install", pkg])


# ---------------------------------------------------------------------------
def step_build_cpp():
    """Compile the C++ ray-tracing engine."""
    print("[1/3] Compiling C++ engine ...")

    shutil.rmtree(BUILD_DIR, ignore_errors=True)

    cmake_args = ["cmake", "-S", str(ROOT), "-B", str(BUILD_DIR),
                  "-DCMAKE_BUILD_TYPE=Release"]
    gen = find_cmake_generator()
    if gen:
        cmake_args += ["-G", gen]

    cmd(cmake_args)
    cmd(["cmake", "--build", str(BUILD_DIR), "--config", "Release"])

    # Locate the binary (multi-config MSVC puts it in a Release/ subfolder)
    candidates = [
        BUILD_DIR / "bin" / BIN_NAME,
        BUILD_DIR / "bin" / "Release" / BIN_NAME,
    ]
    binary = next((c for c in candidates if c.exists()), None)
    if not binary:
        # try a recursive search as last resort
        found = list(BUILD_DIR.rglob(BIN_NAME))
        binary = found[0] if found else None

    if not binary or not binary.exists():
        sys.exit(f"ERROR: C++ binary not found (looked for {BIN_NAME})")
    print(f"  OK  {binary}")


def step_pyinstaller():
    """Bundle Python + C++ into a single executable."""
    print(f"\n[2/3] PyInstaller packaging  (platform: {platform.system()}) ...")

    shutil.rmtree(DIST, ignore_errors=True)
    shutil.rmtree(PI_BUILD_DIR, ignore_errors=True)

    cmd([sys.executable, "-m", "PyInstaller",
         "--clean", "--noconfirm", str(SPEC_FILE)])

    # Find the output
    if IS_WIN:
        out = DIST / "optical_raytrace_ui.exe"
    else:
        out = DIST / "optical_raytrace_ui"

    if not out.exists():
        sys.exit(f"ERROR: output not found at {out}")
    size_mb = out.stat().st_size / (1024 * 1024)
    print(f"  OK  {out}  ({size_mb:.1f} MB)")


def step_summary():
    """Print summary & usage."""
    if IS_WIN:
        exe = DIST / "optical_raytrace_ui.exe"
    else:
        exe = DIST / "optical_raytrace_ui"
    name = exe.name

    print(f"\n{'='*50}")
    print(f"  Build complete!")
    print(f"  Distributable: {exe}")
    print(f"{'='*50}")
    print(f"""
  User instructions:
    1. Copy "{name}" to the target machine.
    2. Double-click to launch.
    3. Browser opens at http://127.0.0.1:5000

  Results are saved under:
    ~/.optical_raytrace/outputs/   (macOS / Linux)
    %USERPROFILE%\\.optical_raytrace\\outputs\\  (Windows)
""")


def main():
    print(f"光学追迹 Release Builder — {platform.system()} {platform.machine()}")
    print(f"Python: {sys.version.split()[0]}")
    print()

    install_python_deps()
    step_build_cpp()
    step_pyinstaller()
    step_summary()


if __name__ == "__main__":
    main()
