#!/bin/bash

# 云端 CI 执行（镜像：Maven 3.9.5 + JDK 17）。流水线需保证 JAVA_HOME / PATH 指向 JDK 17。
#
# 部分平台会把本脚本复制到临时目录再执行，此时不能用「脚本所在目录的上一级」作为项目根。
# 解析顺序：MIAOWU_PROJECT_ROOT → WORKSPACE（Jenkins）→ CI_PROJECT_DIR（GitLab）→
# GITHUB_WORKSPACE → 当前 PWD → 自 PWD 向上查找含 backend/pom.xml 的目录 →
# 脚本父目录（源码树内直接执行时）→ 自脚本路径向上查找。
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

is_repo_root() {
  local candidate="$1"
  [ -n "$candidate" ] && [ -f "$candidate/backend/pom.xml" ]
}

search_upwards() {
  local start_dir="$1"
  local current="$start_dir"

  while [ -n "$current" ] && [ "$current" != "/" ]; do
    if is_repo_root "$current"; then
      echo "$(cd "$current" && pwd)"
      return 0
    fi
    current="$(cd "$current/.." && pwd)"
  done

  return 1
}

resolve_project_root() {
  local try script_parent

  for try in \
    "${MIAOWU_PROJECT_ROOT:-}" \
    "${WORKSPACE:-}" \
    "${CI_PROJECT_DIR:-}" \
    "${GITHUB_WORKSPACE:-}" \
    "$PWD"; do
    if [ -n "$try" ] && is_repo_root "$try"; then
      echo "$(cd "$try" && pwd)"
      return 0
    fi
  done

  if search_upwards "$PWD" >/dev/null 2>&1; then
    search_upwards "$PWD"
    return 0
  fi

  script_parent="$(cd "$SCRIPT_DIR/.." && pwd)"
  if is_repo_root "$script_parent"; then
    echo "$script_parent"
    return 0
  fi

  if search_upwards "$SCRIPT_DIR" >/dev/null 2>&1; then
    search_upwards "$SCRIPT_DIR"
    return 0
  fi

  return 1
}

if ! PROJECT_ROOT="$(resolve_project_root)"; then
  echo "无法识别喵屋（miaowu）仓库根目录（需要存在 backend/pom.xml）。"
  echo "当前工作目录: $PWD"
  echo "脚本路径: $0"
  echo "脚本目录: $SCRIPT_DIR"
  echo "可在流水线中设置 MIAOWU_PROJECT_ROOT=/仓库绝对路径，或在检出目录下执行（保证 PWD 或 Jenkins WORKSPACE 指向仓库根）。"
  exit 1
fi

BACKEND_DIR="$PROJECT_ROOT/backend"
BACKEND_JAR="$BACKEND_DIR/target/saas.jar"

echo "========================================="
echo "喵屋（miaowu）生产后端构建"
echo "========================================="
echo "项目目录: $PROJECT_ROOT"
echo "后端目录: $BACKEND_DIR"

if [ ! -d "$BACKEND_DIR" ]; then
  echo "缺少后端工程目录：$BACKEND_DIR"
  exit 1
fi

java -version 2>&1 | head -n 1 || true
echo "JAVA_HOME: ${JAVA_HOME:-（未设置，使用 PATH 中的 java）}"
echo ""

cd "$BACKEND_DIR"
mvn -B -U clean package -DskipTests

if [ ! -f "$BACKEND_JAR" ]; then
  echo "构建失败：未找到产物 $BACKEND_JAR"
  exit 1
fi

echo ""
echo "构建完成: $BACKEND_JAR"
ls -la "$BACKEND_JAR"
