# -*- mode: python ; coding: utf-8 -*-
"""PyInstaller spec for optical_raytrace_ui — cross-platform"""
import sys
from pathlib import Path

_root = Path(SPECPATH).resolve()
_is_win = sys.platform == 'win32'
_bin_name = 'optical_raytrace.exe' if _is_win else 'optical_raytrace'

a = Analysis(
    [str(_root / 'ui' / 'server.py')],
    pathex=[],
    binaries=[],
    datas=[
        (str(_root / 'build' / 'bin' / _bin_name), 'bin'),
        (str(_root / 'ui' / 'templates'), 'templates'),
        (str(_root / 'examples'), 'examples'),
    ],
    hiddenimports=['flask', 'werkzeug', 'jinja2', 'markupsafe', 'itsdangerous', 'click'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
)

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='optical_raytrace_ui',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=None,
)
