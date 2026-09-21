#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BACKEND_DIR="$PROJECT_ROOT/backend"
LOG_DIR="${APP_LOG_ROOT:-$PROJECT_ROOT/logs}"
BACKEND_LOG_FILE="$LOG_DIR/backend.local.log"
BACKEND_PID_FILE="$LOG_DIR/backend.local.pid"
SKIP_BUILD="${SAAS_PAY_SKIP_BUILD:-0}"
JAVA_VERSION="${SAAS_PAY_JAVA_VERSION:-17}"
BACKEND_PORT="9218"
SPRING_PROFILE="local"
BACKEND_JAR="$BACKEND_DIR/target/saas.jar"
STARTUP_COMPLETE=0

mkdir -p "$LOG_DIR"

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

  echo "发现占用端口 ${port} 的 ${label} 进程: $pids"
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
  rm -f "$BACKEND_PID_FILE"
}

trap cleanup_on_exit EXIT

wait_for_backend_ready() {
  local pid="$1"

  for _ in $(seq 1 30); do
    if curl -sS -o /dev/null "http://localhost:${BACKEND_PORT}/" 2>/dev/null; then
      return 0
    fi

    if [ -f "$BACKEND_LOG_FILE" ] && grep -Eq 'APPLICATION FAILED TO START|Application run failed|Exception encountered during context initialization|BeanCreationException|Could not resolve placeholder' "$BACKEND_LOG_FILE"; then
      echo "后端启动失败，日志如下："
      tail -n 120 "$BACKEND_LOG_FILE"
      exit 1
    fi

    if ! kill -0 "$pid" >/dev/null 2>&1; then
      echo "后端进程已退出，日志如下："
      [ -f "$BACKEND_LOG_FILE" ] && tail -n 120 "$BACKEND_LOG_FILE"
      exit 1
    fi

    sleep 1
  done

  echo "后端在预期时间内未就绪，日志如下："
  [ -f "$BACKEND_LOG_FILE" ] && tail -n 120 "$BACKEND_LOG_FILE"
  exit 1
}

echo "========================================="
echo "电子木鱼本地测试后端启动"
echo "========================================="
echo "项目目录: $PROJECT_ROOT"
echo "后端目录: $BACKEND_DIR"

if [ ! -f "$BACKEND_DIR/pom.xml" ]; then
  echo "缺少后端 Maven 工程：$BACKEND_DIR"
  exit 1
fi

echo ""
echo "[1/3] 关闭现有后端..."

if [ -f "$BACKEND_PID_FILE" ]; then
  OLD_PID="$(cat "$BACKEND_PID_FILE" 2>/dev/null || true)"
  if [ -n "${OLD_PID:-}" ] && kill -0 "$OLD_PID" >/dev/null 2>&1; then
    kill "$OLD_PID" >/dev/null 2>&1 || true
    sleep 1
    if kill -0 "$OLD_PID" >/dev/null 2>&1; then
      kill -9 "$OLD_PID" >/dev/null 2>&1 || true
    fi
    echo "已按 PID 文件停止后端: $OLD_PID"
  fi
  rm -f "$BACKEND_PID_FILE"
fi

pkill -f "spring-boot:run.*${BACKEND_DIR}" >/dev/null 2>&1 || true
pkill -f "$BACKEND_JAR" >/dev/null 2>&1 || true
kill_listener_by_port "$BACKEND_PORT" "Java"
rm -f "$BACKEND_PID_FILE"

if [ -z "${JAVA_HOME:-}" ] && command -v /usr/libexec/java_home >/dev/null 2>&1; then
  JAVA_HOME="$(/usr/libexec/java_home -v "$JAVA_VERSION" 2>/dev/null || true)"
fi
if [ -z "${JAVA_HOME:-}" ] && [ -d "/opt/homebrew/opt/openjdk@${JAVA_VERSION}" ]; then
  JAVA_HOME="/opt/homebrew/opt/openjdk@${JAVA_VERSION}/libexec/openjdk.jdk/Contents/Home"
fi
if [ -z "${JAVA_HOME:-}" ] || [ ! -x "$JAVA_HOME/bin/java" ]; then
  echo "启动失败：未找到 JDK ${JAVA_VERSION}"
  echo "请设置 JAVA_HOME 或安装 JDK ${JAVA_VERSION}"
  exit 1
fi
export JAVA_HOME
export PATH="$JAVA_HOME/bin:$PATH"

JAVA_MAJOR_VERSION="$($JAVA_HOME/bin/java -version 2>&1 | sed -n 's/.*version "\([0-9][0-9]*\).*/\1/p' | head -n 1)"
if [ "$JAVA_MAJOR_VERSION" != "17" ]; then
  echo "启动失败：需要 JDK 17，当前 Java 主版本为 ${JAVA_MAJOR_VERSION:-unknown}"
  exit 1
fi

if [ "$SKIP_BUILD" != "1" ]; then
  MAVEN_VERSION="$(mvn -version 2>&1 | sed -n '1s/Apache Maven \([^ ]*\).*/\1/p')"
  case "$MAVEN_VERSION" in
    3.9.*) ;;
    *)
      echo "启动失败：需要 Maven 3.9.x，当前版本为 ${MAVEN_VERSION:-unknown}"
      exit 1
      ;;
  esac

  echo "[2/3] 编译后端服务..."
  : > "$BACKEND_LOG_FILE"
  cd "$BACKEND_DIR"
  mvn -DskipTests clean package 2>&1 | tee -a "$BACKEND_LOG_FILE"
  echo "后端编译成功"
else
  echo "[2/3] 跳过后端编译..."
fi

if [ ! -f "$BACKEND_JAR" ]; then
  echo "启动失败：未找到后端 Jar：$BACKEND_JAR"
  echo "检查日志：$BACKEND_LOG_FILE"
  exit 1
fi

echo "[3/3] 启动本地测试后端..."
echo "  Spring Profile: $SPRING_PROFILE"
echo "  后端端口: $BACKEND_PORT"
echo "  后端日志: $BACKEND_LOG_FILE"

nohup java -jar "$BACKEND_JAR" \
  --server.port="$BACKEND_PORT" \
  --spring.profiles.active="$SPRING_PROFILE" \
  > "$BACKEND_LOG_FILE" 2>&1 &
BACKEND_PID=$!
echo "$BACKEND_PID" > "$BACKEND_PID_FILE"

echo "等待后端服务启动..."
wait_for_backend_ready "$BACKEND_PID"

STARTUP_COMPLETE=1

echo ""
echo "========================================="
echo "本地测试后端启动完成"
echo "========================================="
echo "  后端地址: http://localhost:${BACKEND_PORT}"
echo "  后端 PID: $BACKEND_PID"
echo "  后端日志: $BACKEND_LOG_FILE"
echo "  PID 文件: $BACKEND_PID_FILE"
