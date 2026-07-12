# AGENTS.md — optical_raytrace_cpp

## Build & run

```bash
# Build (C++17, CMake)
cmake -S optical_raytrace_cpp -B optical_raytrace_cpp/build
cmake --build optical_raytrace_cpp/build

# Pre-compiled binary (symlink)
./optical_raytrace_cpp/run                        # reads config.json
./optical_raytrace_cpp/run compute --input <f.json> [--output f.txt] [--print]
./optical_raytrace_cpp/run compute76 --infinity <i.json> --finite <f.json> [--output f.txt]

# Web UI (Flask, port 5000)
python3 optical_raytrace_cpp/ui/server.py
```

## Gotchas

- **No automated tests.** Verification is manual against Zemax output in `checkout/zemax_reference_76.txt`.
- **`saveResultsToCsv` writes fixed-width `.txt`, not CSV.** Column format: `case`, `order`, `name_zh`, `value`, `unit`.
- **Example files with `"PLEASE_FILL..."` refractive indices are not directly runnable.** Use `test_*.json` which have real H-K9L / H-ZF2 data.
- **Sign conventions are strict** — see README symbol table. Radius `inf` = plane; positive radius = center right of vertex; negative object distance = real object.
- **Materials must include `"AIR"`** with all three refractive indices set to 1.0.
- **`run` at project root is a symlink** to `build/bin/optical_raytrace`. The web UI hardcodes this binary path.
- **Web UI writes temp files** to `outputs/_tmp_*.json` at runtime.

## Dependencies

- **C++**: nlohmann/json vendored at `third_party/json.hpp` (single header, no package install).
- **Python**: Flask (`pip install flask`). No other Python deps.
- C++17, compiler with standard library (g++/clang++).

## Distribution (PyInstaller)

```bash
# One-shot: builds C++ binary + packages everything into a single executable
python3 optical_raytrace_cpp/build_release.py

# Output: dist/optical_raytrace_ui  (~10 MB self-contained)
# Users can double-click to run — no Python/compiler needed.
# Results are saved to ~/.optical_raytrace/outputs/
```

- `optical_raytrace.spec` defines the PyInstaller bundle (binary + templates + examples).
- `server.py` auto-detects `sys.frozen` to switch resource paths between dev and bundled mode.
- `.github/workflows/release.yml` builds artifacts for Linux, Windows, and macOS on every push.
- `install.sh` copies to `/usr/local/bin` and creates a `.desktop` launcher entry.

## Project structure

All source in `optical_raytrace_cpp/`:

| Dir | Purpose |
|---|---|
| `include/` | Headers: Models, RayTrace, Aberrations, Paraxial, etc. |
| `src/` | Implementation + `main.cpp` (CLI entry point) |
| `ui/` | Flask web server + Jinja2 templates |
| `examples/` | `.json` optical system definitions |
| `checkout/` | Zemax reference output for manual comparison |
| `third_party/` | nlohmann/json `json.hpp` |
| `build/` | CMake build output (gitignored) |
| `outputs/` | Runtime results + temp files (gitignored) |
