#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_DIR="${APP_LOG_ROOT:-$PROJECT_ROOT/logs}"
BACKEND_LOG_FILE="$LOG_DIR/backend.local.log"
BACKEND_PID_FILE="$LOG_DIR/backend.local.pid"
TUNNEL_LOG_FILE="$LOG_DIR/backend.local.tunnel.log"
TUNNEL_PID_FILE="$LOG_DIR/backend.local.tunnel.pid"
TUNNEL_CONFIG_FILE="$LOG_DIR/application-local-tunnel.yml"
SKIP_BUILD="${SAAS_PAY_SKIP_BUILD:-0}"
JAVA_VERSION="${SAAS_PAY_JAVA_VERSION:-17}"
BACKEND_PORT="${BACKEND_PORT:-9219}"
SPRING_PROFILE="${SPRING_PROFILE:-local}"
TUNNEL_PROTOCOL="${SAAS_PAY_TUNNEL_PROTOCOL:-http2}"
TUNNEL_TARGET_URL="http://localhost:${BACKEND_PORT}"
STARTUP_COMPLETE=0

mkdir -p "$LOG_DIR"
cd "$PROJECT_ROOT/backend"

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

  echo "  发现占用端口 ${port} 的 ${label} 进程: $pids"
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

cleanup_on_exit() {
  if [ "$STARTUP_COMPLETE" = "1" ]; then
    return 0
  fi

  if [ -n "${TUNNEL_PID:-}" ] && kill -0 "$TUNNEL_PID" >/dev/null 2>&1; then
    kill "$TUNNEL_PID" >/dev/null 2>&1 || true
  fi

  rm -f "$TUNNEL_PID_FILE" "$TUNNEL_CONFIG_FILE"
}

trap cleanup_on_exit EXIT
wait_for_backend_ready() {
  local pid="$1"

  for _ in $(seq 1 30); do
    if curl -sS -o /dev/null "http://localhost:${BACKEND_PORT}/" 2>/dev/null; then
      return 0
    fi

    if [ -f "$BACKEND_LOG_FILE" ] && grep -Eq 'APPLICATION FAILED TO START|Application run failed|Exception encountered during context initialization|BeanCreationException|Could not resolve placeholder' "$BACKEND_LOG_FILE"; then
      echo "❌ 后端启动失败，日志如下："
      tail -n 120 "$BACKEND_LOG_FILE"
      exit 1
    fi

    if ! kill -0 "$pid" >/dev/null 2>&1; then
      echo "❌ 后端进程已退出，日志如下："
      [ -f "$BACKEND_LOG_FILE" ] && tail -n 120 "$BACKEND_LOG_FILE"
      exit 1
    fi

    sleep 1
  done

  echo "❌ 后端在预期时间内未就绪，日志如下："
  [ -f "$BACKEND_LOG_FILE" ] && tail -n 120 "$BACKEND_LOG_FILE"
  exit 1
}

echo "========================================="
echo "SaaS Pay Base 本地测试后端启动"
echo "========================================="

echo ""
echo "[1/3] 关闭现有后端与隧道..."

if [ -f "$BACKEND_PID_FILE" ]; then
  OLD_PID="$(cat "$BACKEND_PID_FILE" 2>/dev/null || true)"
  if [ -n "${OLD_PID:-}" ] && kill -0 "$OLD_PID" >/dev/null 2>&1; then
    kill "$OLD_PID" >/dev/null 2>&1 || true
    sleep 1
    if kill -0 "$OLD_PID" >/dev/null 2>&1; then
      kill -9 "$OLD_PID" >/dev/null 2>&1 || true
    fi
    echo "stopped backend by pid file: $OLD_PID"
  fi
  rm -f "$BACKEND_PID_FILE"
fi

pkill -f "spring-boot:run.*miaowu/backend" >/dev/null 2>&1 || true
pkill -f "miaowu/backend/target/saas.jar" >/dev/null 2>&1 || true
pkill -f "saas\\.jar" >/dev/null 2>&1 || true
kill_listener_by_port "$BACKEND_PORT" "Java"
if [ -f "$TUNNEL_PID_FILE" ]; then
  OLD_TUNNEL_PID="$(cat "$TUNNEL_PID_FILE" 2>/dev/null || true)"
  if [ -n "${OLD_TUNNEL_PID:-}" ] && kill -0 "$OLD_TUNNEL_PID" >/dev/null 2>&1; then
    kill "$OLD_TUNNEL_PID" >/dev/null 2>&1 || true
    sleep 1
    if kill -0 "$OLD_TUNNEL_PID" >/dev/null 2>&1; then
      kill -9 "$OLD_TUNNEL_PID" >/dev/null 2>&1 || true
    fi
    echo "stopped backend tunnel by pid file: $OLD_TUNNEL_PID"
  fi
  rm -f "$TUNNEL_PID_FILE"
fi
pkill -f "cloudflared.*$TUNNEL_TARGET_URL" >/dev/null 2>&1 || true
echo "  旧进程已停止"

rm -f "$BACKEND_PID_FILE"
rm -f "$TUNNEL_PID_FILE" "$TUNNEL_CONFIG_FILE"

if [ -z "${JAVA_HOME:-}" ] && command -v /usr/libexec/java_home >/dev/null 2>&1; then
  JAVA_HOME="$(/usr/libexec/java_home -v "$JAVA_VERSION" 2>/dev/null || true)"
fi
if [ -z "${JAVA_HOME:-}" ] && [ -d "/opt/homebrew/opt/openjdk@${JAVA_VERSION}" ]; then
  JAVA_HOME="/opt/homebrew/opt/openjdk@${JAVA_VERSION}/libexec/openjdk.jdk/Contents/Home"
fi
if [ -z "${JAVA_HOME:-}" ] || [ ! -x "$JAVA_HOME/bin/java" ]; then
  echo "failed to start backend: JDK ${JAVA_VERSION} not found"
  echo "set JAVA_HOME or install JDK ${JAVA_VERSION}"
  exit 1
fi
export JAVA_HOME
export PATH="$JAVA_HOME/bin:$PATH"

if ! command -v cloudflared >/dev/null 2>&1; then
  echo "failed to start backend: cloudflared not found"
  echo "install it first: brew install cloudflared"
  exit 1
fi

echo "  Java环境: $JAVA_HOME"
echo "  Spring Profile: $SPRING_PROFILE"
echo "  后端端口: $BACKEND_PORT"
echo "  Tunnel目标地址: $TUNNEL_TARGET_URL"
echo ""

if [ "$SKIP_BUILD" != "1" ]; then
  echo "[2/3] 编译后端服务..."
  : > "$BACKEND_LOG_FILE"
  mvn -DskipTests clean package 2>&1 | tee -a "$BACKEND_LOG_FILE"
  echo "  后端编译成功"
else
  echo "[2/3] 跳过后端编译..."
  echo "  skip backend build enabled"
fi

BACKEND_JAR="$PROJECT_ROOT/backend/target/saas.jar"
if [ ! -f "$BACKEND_JAR" ]; then
  echo "failed to start backend: jar not found under backend/target"
  echo "check log: $BACKEND_LOG_FILE"
  exit 1
fi

rm -f "$TUNNEL_LOG_FILE"
echo ""
echo "[3/3] 启动本地测试后端..."
echo "  启动公网隧道: ${TUNNEL_TARGET_URL}"
echo "  隧道日志: $TUNNEL_LOG_FILE"
nohup cloudflared tunnel --url "$TUNNEL_TARGET_URL" --protocol "$TUNNEL_PROTOCOL" > "$TUNNEL_LOG_FILE" 2>&1 &
TUNNEL_PID=$!
echo "$TUNNEL_PID" > "$TUNNEL_PID_FILE"

echo "  等待公网回调地址生成..."
PUBLIC_BASE_URL=""
HAS_CONNECTION=0
for _ in $(seq 1 60); do
  if [ -f "$TUNNEL_LOG_FILE" ]; then
    if grep -q "failed to request quick Tunnel" "$TUNNEL_LOG_FILE"; then
      echo "cloudflared failed to request quick tunnel"
      echo "check log: $TUNNEL_LOG_FILE"
      exit 1
    fi

    if grep -Eq 'Registered tunnel connection|Connection .* registered' "$TUNNEL_LOG_FILE"; then
      HAS_CONNECTION=1
    fi

    PUBLIC_BASE_URL="$(grep -Eo 'https://[-a-z0-9]+\.trycloudflare\.com' "$TUNNEL_LOG_FILE" | grep -v 'https://api.trycloudflare.com' | head -n 1 || true)"
    if [ -n "$PUBLIC_BASE_URL" ] && [ "$HAS_CONNECTION" = "1" ]; then
      break
    fi
  fi

  if ! kill -0 "$TUNNEL_PID" >/dev/null 2>&1; then
    echo "backend tunnel process exited during startup"
    echo "check log: $TUNNEL_LOG_FILE"
    exit 1
  fi

  sleep 1
done

if [ -z "$PUBLIC_BASE_URL" ] || [ "$HAS_CONNECTION" != "1" ]; then
  echo "backend tunnel did not become ready within 60 seconds"
  echo "check log: $TUNNEL_LOG_FILE"
  exit 1
fi

cat > "$TUNNEL_CONFIG_FILE" <<EOF
jeepay:
  notify-url: "${PUBLIC_BASE_URL}/api/pay/notify"
EOF

FRONTEND_ENV_LOCAL="$PROJECT_ROOT/frontend/.env.development.local"
cat > "$FRONTEND_ENV_LOCAL" <<ENVEOF
# 本地开发覆盖（此文件不提交 Git，已被 .gitignore 排除）
# Vite 加载优先级：.env.development.local > .env.development > .env
#
# ——— 此文件由 start-local-test-backend.sh 自动生成 ———
# 每次运行后端脚本后，下面三行会更新为最新的 cloudflared 公网地址，但默认带 # 注释。
# 需要真机 / 公网 API 时，请手动去掉 API / UPLOAD 两行开头的 #。
# H5/模拟器连本机后端一般用 .env.development 里的 localhost，可保持注释不动。
#
# VITE_API_BASE_URL=${PUBLIC_BASE_URL}/api/v1
# VITE_UPLOAD_BASE_URL=${PUBLIC_BASE_URL}/api/v1
# VITE_CDN_BASE_URL=https://miaowu-images.zhcmqtt.top
ENVEOF

echo "  公网回调地址: $PUBLIC_BASE_URL"
echo "  回调通知地址: ${PUBLIC_BASE_URL}/api/pay/notify"
echo "  前端公网环境: $FRONTEND_ENV_LOCAL"
echo "  运行时覆盖配置: $TUNNEL_CONFIG_FILE"
echo "  后端日志: $BACKEND_LOG_FILE"
echo "  启动后端中..."

nohup java -jar "$BACKEND_JAR" --spring.profiles.active="$SPRING_PROFILE" --spring.config.additional-location="file:${TUNNEL_CONFIG_FILE}" > "$BACKEND_LOG_FILE" 2>&1 &
BACKEND_PID=$!
echo "$BACKEND_PID" > "$BACKEND_PID_FILE"

echo "  等待后端服务启动..."
wait_for_backend_ready "$BACKEND_PID"

STARTUP_COMPLETE=1

echo ""
echo "========================================="
echo "本地测试后端启动完成"
echo "========================================="
echo "  后端地址: http://localhost:${BACKEND_PORT}"
echo "  Spring Profile: $SPRING_PROFILE"
echo "  后端 PID: $BACKEND_PID"
echo "  后端日志: $BACKEND_LOG_FILE"
echo "  Tunnel PID: $TUNNEL_PID"
echo "  Tunnel日志: $TUNNEL_LOG_FILE"
echo "  Tunnel PID文件: $TUNNEL_PID_FILE"
echo "  公网回调地址: $PUBLIC_BASE_URL"
echo "  回调通知地址: ${PUBLIC_BASE_URL}/api/pay/notify"
echo "  前端公网环境: $FRONTEND_ENV_LOCAL"
echo "  运行时覆盖配置: $TUNNEL_CONFIG_FILE"
