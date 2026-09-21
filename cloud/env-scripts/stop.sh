#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BACKEND_DIR="$PROJECT_ROOT/backend"
LOG_DIR="${APP_LOG_ROOT:-$PROJECT_ROOT/logs}"
BACKEND_PID_FILE="$LOG_DIR/backend.local.pid"
BACKEND_PORT="9218"
BACKEND_JAR="$BACKEND_DIR/target/saas.jar"

kill_listener_by_port() {
  local port="$1"
  local label="$2"
  local pids=""

  if command -v lsof >/dev/null 2>&1; then
    pids="$(lsof -tiTCP:"$port" -sTCP:LISTEN 2>/dev/null || true)"
  elif command -v netstat >/dev/null 2>&1; then
    pids="$(netstat -anv -p tcp 2>/dev/null | awk -v port=".$port" '$4 ~ port && $6 == "LISTEN" {print $9}' | sort -u)"
  fi

  if [ -z "$pids" ]; then
    return 0
  fi

  echo "停止 $label 端口监听 $port: $pids"
  for pid in $pids; do
    kill "$pid" >/dev/null 2>&1 || true
  done
  sleep 1
  for pid in $pids; do
    if kill -0 "$pid" >/dev/null 2>&1; then
      kill -9 "$pid" >/dev/null 2>&1 || true
    fi
  done
}

stop_by_pid_file() {
  local pid_file="$1"
  local name="$2"

  if [ -f "$pid_file" ]; then
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"
    if [ -n "$pid" ] && kill -0 "$pid" >/dev/null 2>&1; then
      kill "$pid" >/dev/null 2>&1 || true
      sleep 1
      if kill -0 "$pid" >/dev/null 2>&1; then
        kill -9 "$pid" >/dev/null 2>&1 || true
      fi
      echo "已按 PID 文件停止 $name: $pid"
    fi
    rm -f "$pid_file"
  fi
}

mkdir -p "$LOG_DIR"

echo "========================================="
echo "停止电子木鱼本地后端"
echo "  项目目录: $PROJECT_ROOT"
echo "  后端端口: $BACKEND_PORT"
echo "========================================="

stop_by_pid_file "$BACKEND_PID_FILE" "backend"
pkill -f "spring-boot:run.*${BACKEND_DIR}" >/dev/null 2>&1 || true
pkill -f "$BACKEND_JAR" >/dev/null 2>&1 || true
kill_listener_by_port "$BACKEND_PORT" "backend"

echo ""
echo "========================================="
echo "本地后端已停止"
echo "========================================="
