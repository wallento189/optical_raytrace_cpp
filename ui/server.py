#!/usr/bin/env python3
"""光学追迹 Web 界面 — Flask 后端"""
import os, sys, json, subprocess, webbrowser, threading, atexit, signal
from pathlib import Path
from flask import Flask, render_template, request, jsonify, send_file

_FROZEN = getattr(sys, 'frozen', False)

_IS_WIN = sys.platform == 'win32'
_BIN_NAME = "optical_raytrace.exe" if _IS_WIN else "optical_raytrace"

if _FROZEN:
    _BUNDLE_DIR = Path(sys._MEIPASS)
    BINARY = _BUNDLE_DIR / "bin" / _BIN_NAME
    EXAMPLES = _BUNDLE_DIR / "examples"
    BASE = _BUNDLE_DIR
    OUTPUTS = Path.home() / ".optical_raytrace" / "outputs"
else:
    BASE = Path(__file__).resolve().parent.parent
    BINARY = BASE / "build" / "bin" / _BIN_NAME
    EXAMPLES = BASE / "examples"
    OUTPUTS = BASE / "outputs"

OUTPUTS.mkdir(parents=True, exist_ok=True)

app = Flask(
    __name__,
    template_folder=str(BASE / "templates"),
    static_folder=str(BASE / "static"),
)

@app.errorhandler(Exception)
def handle_exception(e):
    from werkzeug.exceptions import HTTPException
    if isinstance(e, HTTPException):
        return jsonify({"error": str(e)}), e.code
    import traceback
    return jsonify({"error": str(e), "trace": traceback.format_exc()}), 500

def run_binary(mode, args):
    cmd = [str(BINARY), mode] + args
    kwargs = dict(capture_output=True, text=True, timeout=60, cwd=str(BASE))
    if _IS_WIN:
        kwargs['creationflags'] = subprocess.CREATE_NO_WINDOW
    result = subprocess.run(cmd, **kwargs)
    return result.stdout, result.stderr, result.returncode

@app.route("/")
def index():
    examples = sorted([f.name for f in EXAMPLES.glob("*.json")]) if EXAMPLES.exists() else []
    outputs = sorted([f.name for f in OUTPUTS.glob("*.txt")])
    return render_template("index.html", examples=examples, outputs=outputs)

@app.route("/api/load_config")
def load_config():
    fname = request.args.get("file", "")
    path = EXAMPLES / fname
    if not path.exists():
        path = os.path.join(str(BASE), fname)
    if not os.path.exists(path):
        return jsonify({"error": f"File not found: {fname}"}), 404
    with open(path) as f:
        return jsonify(json.load(f))

@app.route("/api/save_config", methods=["POST"])
def save_config():
    data = request.get_json()
    fname = data.pop("_filename", "custom.json")
    path = OUTPUTS / fname
    with open(path, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    return jsonify({"ok": True, "path": str(path)})

@app.route("/api/run", methods=["POST"])
def run():
    data = request.get_json()
    mode = data.get("mode", "compute76")
    print_config = data.get("print", False)
    config = data.get("config", {})

    inf_path = OUTPUTS / "_tmp_infinity.json"
    fin_path = OUTPUTS / "_tmp_finite.json"
    out_path = OUTPUTS / "result_76.txt"

    if mode == "compute":
        inp = config.get("input") or data.get("input", "")
        if inp:
            if not os.path.isabs(inp):
                inp = str(BASE / inp)
        else:
            inp = str(EXAMPLES / "test_infinity.json") if (EXAMPLES / "test_infinity.json").exists() else str(EXAMPLES / "test_infinity.json")
        args = ["--input", inp]
        if print_config:
            args.append("--print")
        out = config.get("output", "")
        if out:
            args += ["--output", str(BASE / out)]
        stdout, stderr, rc = run_binary("compute", args)
        return jsonify({"ok": rc == 0, "stdout": stdout, "stderr": stderr, "mode": "compute"})

    elif mode == "compute76":
        inf = config.get("infinity", "")
        fin = config.get("finite", "")
        out = config.get("output", str(OUTPUTS / "result_76.txt"))

        if inf:
            if not os.path.isabs(inf): inf = str(BASE / inf)
        else:
            inf = str(inf_path)
            with open(inf_path, "w") as f: f.write(data.get("infinity_json", "{}"))

        if fin:
            if not os.path.isabs(fin): fin = str(BASE / fin)
        else:
            fin = str(fin_path)
            with open(fin_path, "w") as f: f.write(data.get("finite_json", "{}"))

        if not os.path.isabs(out): out = str(BASE / out)

        args = ["--infinity", inf, "--finite", fin, "--output", out]
        if print_config: args.append("--print")
        stdout, stderr, rc = run_binary("compute76", args)

        if rc != 0:
            return jsonify({"ok": False, "stdout": stdout, "stderr": stderr, "mode": "compute76"})

        return jsonify({"ok": True, "stdout": stdout, "stderr": stderr, "output": out, "mode": "compute76"})

    return jsonify({"ok": False, "error": f"Unknown mode: {mode}"})

@app.route("/api/results")
def get_results():
    fname = request.args.get("file", str(OUTPUTS / "result_76.txt"))
    if not os.path.isabs(fname):
        fname = str(BASE / fname)
    if not os.path.exists(fname):
        return jsonify({"error": f"No results file: {fname}"}), 404

    rows = []
    with open(fname) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("--") or line.startswith("case"):
                continue
            tokens = line.split()
            if len(tokens) < 4:
                continue

            case_name = tokens[0]
            order = tokens[1]

            val_idx = None
            for i in range(len(tokens) - 1, 1, -1):
                t = tokens[i]
                if t in ('NaN', 'Inf', '-Inf'):
                    val_idx = i; break
                try: float(t); val_idx = i; break
                except ValueError: continue

            if val_idx is None: continue

            value = tokens[val_idx]
            unit = tokens[val_idx + 1] if val_idx + 1 < len(tokens) else ""
            name_zh = " ".join(tokens[2:val_idx])

            rows.append({
                "case": case_name,
                "order": order,
                "name": name_zh,
                "value": value,
                "unit": unit
            })
    return jsonify({"rows": rows, "file": fname})

@app.route("/api/download")
def download():
    fname = request.args.get("file", "")
    if not os.path.isabs(fname):
        fname = str(BASE / fname)
    if os.path.exists(fname):
        return send_file(fname, as_attachment=True, download_name=os.path.basename(fname))
    return jsonify({"error": "not found"}), 404

@app.route("/api/preview", methods=["POST"])
def preview():
    data = request.get_json()
    tmp = OUTPUTS / "_preview.json"
    with open(tmp, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    out, err, rc = run_binary("compute", ["--input", str(tmp), "--print"])
    return jsonify({"ok": rc == 0, "stdout": out, "stderr": err})

if __name__ == "__main__":
    url = "http://127.0.0.1:5000"
    print(f"光学追迹 Web UI — {url}")
    print(f"Binary: {BINARY}   (exists: {BINARY.exists()})")
    if _FROZEN:
        print(f"数据目录: {OUTPUTS}")
    threading.Timer(0.8, lambda: webbrowser.open(url)).start()
    app.run(debug=not _FROZEN, host="127.0.0.1", port=5000)
