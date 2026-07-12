#!/bin/bash
# 启动光学追迹 Web UI
# cd "$(dirname "$0")/.."
mkdir -p outputs
echo "== 光学追迹 Web UI =="
echo "  打开浏览器访问: http://127.0.0.1:5000"
echo ""
python3 ui/server.py
