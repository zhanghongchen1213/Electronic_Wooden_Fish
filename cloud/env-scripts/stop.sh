#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_DIR="${APP_LOG_ROOT:-$PROJECT_ROOT/logs}"
BACKEND_PID_FILE="$LOG_DIR/backend.local.pid"
FRONTEND_PID_FILE="$LOG_DIR/frontend.local.pid"
TUNNEL_PID_FILE="$LOG_DIR/backend.local.tunnel.pid"
TUNNEL_CONFIG_FILE="$LOG_DIR/application-local-tunnel.yml"
BACKEND_PORT="${BACKEND_PORT:-9219}"
FRONTEND_PORT="${FRONTEND_PORT:-4310}"
TUNNEL_TARGET_URL="http://localhost:${BACKEND_PORT}"

kill_listener_by_port() {
  local port="$1"
  local label="$2"
  local pids=""

  if command -v lsof >/dev/null 2>&1; then
    pids="$(lsof -tiTCP:"$port" -sTCP:LISTEN 2>/dev/null || true)"
  fi

  if [ -z "$pids" ]; then
    return 0
  fi

  echo "stopping $label listener on port $port: $pids"
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
      echo "stopped $name by pid file: $pid"
    fi
    rm -f "$pid_file"
  fi
}

mkdir -p "$LOG_DIR"

echo "========================================="
echo "停止 Miaowu 本地前端与后端"
echo "  项目目录: $PROJECT_ROOT"
echo "  后端端口: $BACKEND_PORT  前端端口: $FRONTEND_PORT"
echo "========================================="

echo ""
echo "停止后端服务..."

stop_by_pid_file "$BACKEND_PID_FILE" "backend"
pkill -f "spring-boot:run.*${PROJECT_ROOT}/backend" >/dev/null 2>&1 || true
pkill -f "${PROJECT_ROOT}/backend/target/saas.jar" >/dev/null 2>&1 || true
kill_listener_by_port "$BACKEND_PORT" "backend"

echo ""
echo "停止前端服务..."
stop_by_pid_file "$FRONTEND_PID_FILE" "frontend"
pkill -f "vite.*${PROJECT_ROOT}/frontend" >/dev/null 2>&1 || true
pkill -f "${PROJECT_ROOT}/frontend/node_modules/.bin/vite" >/dev/null 2>&1 || true
pkill -f "vite --host 0.0.0.0 --port ${FRONTEND_PORT}" >/dev/null 2>&1 || true
kill_listener_by_port "$FRONTEND_PORT" "frontend"

echo ""
echo "停止公网隧道..."
stop_by_pid_file "$TUNNEL_PID_FILE" "backend tunnel"
pkill -f "cloudflared.*${TUNNEL_TARGET_URL}" >/dev/null 2>&1 || true

rm -f "$TUNNEL_CONFIG_FILE"

echo ""
echo "========================================="
echo "所有服务已停止"
echo "========================================="
