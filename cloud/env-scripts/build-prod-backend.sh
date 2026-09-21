#!/bin/bash

# Huawei Cloud 或其它 Linux CI 执行：Maven 3.9.x + JDK 17。
# 脚本只依赖通用 shell、Java 和 Maven，不绑定具体 CI 产品。
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

is_project_root() {
  local candidate="$1"
  [ -n "$candidate" ] && [ -f "$candidate/cloud/backend/pom.xml" ]
}

search_upwards() {
  local start_dir="$1"
  local current="$start_dir"

  while [ -n "$current" ] && [ "$current" != "/" ]; do
    if is_project_root "$current"; then
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
    "${EWF_PROJECT_ROOT:-}" \
    "${WORKSPACE:-}" \
    "${CI_PROJECT_DIR:-}" \
    "${GITHUB_WORKSPACE:-}"; do
    if [ -n "$try" ] && is_project_root "$try"; then
      echo "$(cd "$try" && pwd)"
      return 0
    fi
  done

  if search_upwards "$SCRIPT_DIR" >/dev/null 2>&1; then
    search_upwards "$SCRIPT_DIR"
    return 0
  fi

  if search_upwards "$PWD" >/dev/null 2>&1; then
    search_upwards "$PWD"
    return 0
  fi

  script_parent="$(cd "$SCRIPT_DIR/../.." && pwd)"
  if is_project_root "$script_parent"; then
    echo "$script_parent"
    return 0
  fi

  return 1
}

if ! PROJECT_ROOT="$(resolve_project_root)"; then
  echo "无法识别 EWF 仓库根目录（需要存在 cloud/backend/pom.xml）。"
  echo "当前工作目录: $PWD"
  echo "脚本路径: $0"
  echo "脚本目录: $SCRIPT_DIR"
  exit 1
fi

BACKEND_DIR="$PROJECT_ROOT/cloud/backend"
BACKEND_JAR="$BACKEND_DIR/target/saas.jar"
BACKEND_CHECKSUM="$BACKEND_JAR.sha256"

echo "========================================="
echo "电子木鱼生产后端构建"
echo "========================================="
echo "项目目录: $PROJECT_ROOT"
echo "后端目录: $BACKEND_DIR"

if [ ! -d "$BACKEND_DIR" ] || [ ! -f "$BACKEND_DIR/pom.xml" ]; then
  echo "缺少后端 Maven 工程：$BACKEND_DIR"
  exit 1
fi

java -version 2>&1 | head -n 1 || true
echo "JAVA_HOME: ${JAVA_HOME:-（未设置，使用 PATH 中的 java）}"
echo ""

JAVA_MAJOR_VERSION="$(java -version 2>&1 | sed -n 's/.*version "\([0-9][0-9]*\).*/\1/p' | head -n 1)"
if [ "$JAVA_MAJOR_VERSION" != "17" ]; then
  echo "构建失败：需要 JDK 17，当前 Java 主版本为 ${JAVA_MAJOR_VERSION:-unknown}"
  exit 1
fi

MAVEN_VERSION="$(mvn -version 2>&1 | sed -n '1s/Apache Maven \([^ ]*\).*/\1/p')"
case "$MAVEN_VERSION" in
  3.9.*) ;;
  *)
    echo "构建失败：需要 Maven 3.9.x，当前版本为 ${MAVEN_VERSION:-unknown}"
    exit 1
    ;;
esac

cd "$BACKEND_DIR"
mvn -B -U clean package -DskipTests

if [ ! -f "$BACKEND_JAR" ]; then
  echo "构建失败：未找到产物 $BACKEND_JAR"
  exit 1
fi

if command -v sha256sum >/dev/null 2>&1; then
  (cd "$(dirname "$BACKEND_JAR")" && sha256sum "$(basename "$BACKEND_JAR")") > "$BACKEND_CHECKSUM"
elif command -v shasum >/dev/null 2>&1; then
  (cd "$(dirname "$BACKEND_JAR")" && shasum -a 256 "$(basename "$BACKEND_JAR")") > "$BACKEND_CHECKSUM"
else
  echo "构建失败：缺少 sha256sum 或 shasum，无法生成产物校验文件"
  exit 1
fi

echo ""
echo "构建完成: $BACKEND_JAR"
echo "校验文件: $BACKEND_CHECKSUM"
ls -la "$BACKEND_JAR"
ls -la "$BACKEND_CHECKSUM"
