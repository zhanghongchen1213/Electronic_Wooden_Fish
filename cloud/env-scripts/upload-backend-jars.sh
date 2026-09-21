#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

is_project_root() {
  local candidate="$1"
  [ -n "$candidate" ] && [ -d "$candidate/backend" ] && [ -d "$candidate/frontend" ]
}

search_upwards() {
  local start_dir="$1"
  local current="$start_dir"

  while [ -n "$current" ] && [ "$current" != "/" ]; do
    if is_project_root "$current"; then
      echo "$current"
      return 0
    fi
    current="$(cd "$current/.." && pwd)"
  done

  return 1
}

resolve_project_root() {
  if [ -n "${SAAS_PAY_PROJECT_ROOT:-}" ] && is_project_root "$SAAS_PAY_PROJECT_ROOT"; then
    echo "$SAAS_PAY_PROJECT_ROOT"
    return 0
  fi

  if is_project_root "$PWD"; then
    echo "$PWD"
    return 0
  fi

  if search_upwards "$SCRIPT_DIR" >/dev/null 2>&1; then
    search_upwards "$SCRIPT_DIR"
    return 0
  fi

  if search_upwards "$PWD" >/dev/null 2>&1; then
    search_upwards "$PWD"
    return 0
  fi

  return 1
}

if ! ROOT="$(resolve_project_root)"; then
  echo "无法自动识别 saas-pay-base 项目根目录。"
  echo "当前工作目录: $PWD"
  echo "脚本目录: $SCRIPT_DIR"
  echo "请在执行前设置 SAAS_PAY_PROJECT_ROOT=/您的/saas-pay-base/项目根目录"
  exit 1
fi

if ! command -v scp >/dev/null 2>&1; then
  echo "未检测到 scp，请先安装或检查系统环境。"
  exit 1
fi

if ! command -v ssh >/dev/null 2>&1; then
  echo "未检测到 ssh，请先安装或检查系统环境。"
  exit 1
fi

if ! command -v mktemp >/dev/null 2>&1; then
  echo "未检测到 mktemp，请先安装或检查系统环境。"
  exit 1
fi

SSH_HOST="${SSH_HOST:-113.44.43.204}"
SSH_USER="${SSH_USER:-root}"
SSH_PASSWORD="${SSH_PASSWORD:-Qkwdnwjshm123@}"
REMOTE_DIR="${REMOTE_DIR:-/www/wwwroot/miaowu}"

if [ -z "$SSH_HOST" ]; then
  echo "请设置环境变量 SSH_HOST（云服务器地址）。"
  exit 1
fi

if [ -z "$SSH_PASSWORD" ]; then
  echo "请设置环境变量 SSH_PASSWORD（SSH 登录密码）。"
  echo "若使用密钥登录，请改用 ssh-agent / 本机已配置的密钥，并自行调整本脚本认证方式。"
  exit 1
fi
BACKEND_JAR="$ROOT/backend/target/saas.jar"

if [ ! -f "$BACKEND_JAR" ]; then
  echo "缺少后端 jar：$BACKEND_JAR"
  exit 1
fi

SSH_OPTS=(
  -o StrictHostKeyChecking=no
  -o UserKnownHostsFile=/dev/null
)

USE_SSHPASS=0
ASKPASS_SCRIPT=""

cleanup() {
  if [ -n "$ASKPASS_SCRIPT" ] && [ -f "$ASKPASS_SCRIPT" ]; then
    rm -f "$ASKPASS_SCRIPT"
  fi
}

trap cleanup EXIT

if command -v sshpass >/dev/null 2>&1; then
  USE_SSHPASS=1
else
  if ! command -v setsid >/dev/null 2>&1; then
    echo "未检测到 setsid。当前环境既没有 sshpass，也没有 setsid，无法自动完成密码登录上传。"
    exit 1
  fi
  ASKPASS_SCRIPT="$(mktemp "${TMPDIR:-/tmp}/saas-pay-askpass.XXXXXX")"
  cat > "$ASKPASS_SCRIPT" <<EOF
#!/bin/sh
printf '%s\n' '$SSH_PASSWORD'
EOF
  chmod 700 "$ASKPASS_SCRIPT"
fi

run_ssh() {
  if [ "$USE_SSHPASS" = "1" ]; then
    sshpass -p "$SSH_PASSWORD" ssh "${SSH_OPTS[@]}" "$@"
  else
    DISPLAY="${DISPLAY:-:0}" SSH_ASKPASS="$ASKPASS_SCRIPT" SSH_ASKPASS_REQUIRE=force \
      setsid ssh "${SSH_OPTS[@]}" "$@" < /dev/null
  fi
}

run_scp() {
  if [ "$USE_SSHPASS" = "1" ]; then
    sshpass -p "$SSH_PASSWORD" scp "${SSH_OPTS[@]}" "$@"
  else
    DISPLAY="${DISPLAY:-:0}" SSH_ASKPASS="$ASKPASS_SCRIPT" SSH_ASKPASS_REQUIRE=force \
      setsid scp "${SSH_OPTS[@]}" "$@" < /dev/null
  fi
}

echo "========================================="
echo "上传 SaaS 后端 Jar 到云端"
echo "========================================="
echo "项目目录: $ROOT"
echo "目标主机: $SSH_USER@$SSH_HOST"
echo "远端目录: $REMOTE_DIR"
if [ "$USE_SSHPASS" = "1" ]; then
  echo "认证方式: sshpass"
else
  echo "认证方式: SSH_ASKPASS"
fi

echo ""
echo "[1/2] 检查并创建远端目录..."
run_ssh "$SSH_USER@$SSH_HOST" \
  "mkdir -p '$REMOTE_DIR'"
echo "  远端目录已就绪"

echo ""
echo "[2/2] 上传后端 jar..."
run_scp "$BACKEND_JAR" \
  "$SSH_USER@$SSH_HOST:$REMOTE_DIR/"
echo "  已上传: $BACKEND_JAR"

echo ""
echo "========================================="
echo "上传完成"
echo "========================================="
echo "  远端目录: $REMOTE_DIR"
