# AGENTS.md — optical_raytrace_cpp

## Build & run

```bash
# Build (C++17, CMake)
cmake -S . -B build
cmake --build build

# Run
./build/bin/optical_raytrace                        # reads config.json
./build/bin/optical_raytrace compute --input <f.json> [--output f.txt] [--print]
./build/bin/optical_raytrace compute76 --infinity <i.json> --finite <f.json> [--output f.txt]
```

## Gotchas

- **No automated tests.** Verification is manual against Zemax output in `checkout/zemax_reference_76.txt`.
- **`saveResultsToCsv` writes fixed-width `.txt`, not CSV.** Column format: `case`, `order`, `name_zh`, `value`, `unit`.
- **Example files with `"PLEASE_FILL..."` refractive indices are not directly runnable.** Use `test_*.json` which have real H-K9L / H-ZF2 data.
- **Sign conventions are strict** — see README symbol table. Radius `inf` = plane; positive radius = center right of vertex; negative object distance = real object.
- **Materials must include `"AIR"`** with all three refractive indices set to 1.0.

## Dependencies

- **C++**: nlohmann/json vendored at `third_party/json.hpp` (single header, no package install).
- **Python**: Flask (`pip install flask`). No other Python deps.
- C++17, compiler with standard library (g++/clang++/MSVC).

## Distribution (PyInstaller)

```bash
# One-shot: builds C++ binary + packages everything into a single executable
python3 build_release.py

# Output: dist/optical_raytrace_ui  (~10 MB self-contained)
# Users can double-click to launch — no Python/compiler needed.
```

- `optical_raytrace.spec` defines the PyInstaller bundle (binary + templates + examples).
- `server.py` auto-detects `sys.frozen` to switch resource paths between dev and bundled mode.

## CI/CD (GitHub Actions)

- `.github/workflows/release.yml` builds PyInstaller artifacts for Linux, Windows, and macOS on every push.
- Uses `actions/setup-python@v5` + `actions/checkout@v4`.

## Project structure

| Dir | Purpose |
|---|---|
| `include/` | Headers: Models, RayTrace, Aberrations, Paraxial, etc. |
| `src/` | Implementation + `main.cpp` (CLI entry point) |
| `ui/` | Flask web server + Jinja2 templates |
| `examples/` | `.json` optical system definitions |
| `checkout/` | Zemax reference output for manual comparison |
| `third_party/` | nlohmann/json `json.hpp` |
| `build/` | CMake build output (gitignored) |
| `outputs/` | Runtime results (gitignored) |
