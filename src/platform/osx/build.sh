#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$SCRIPT_DIR/OpenLara.xcodeproj"
BUILD_DIR="$SCRIPT_DIR/build"

CONFIG="Release"
ACTION="build"

usage() {
    echo "Usage: $0 [options]"
    echo "  -d, --debug     Debug 빌드 (기본값: Release)"
    echo "  -c, --clean     빌드 전 clean 실행"
    echo "  -r, --run       빌드 후 앱 실행"
    echo "  -h, --help      도움말"
    exit 0
}

RUN=0
CLEAN=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--debug)   CONFIG="Debug"; shift ;;
        -c|--clean)   CLEAN=1; shift ;;
        -r|--run)     RUN=1; shift ;;
        -h|--help)    usage ;;
        *) echo "알 수 없는 옵션: $1"; usage ;;
    esac
done

echo "==> 빌드 설정: $CONFIG"
echo "==> 프로젝트: $PROJECT"
echo "==> 출력 디렉토리: $BUILD_DIR/$CONFIG"

if [[ $CLEAN -eq 1 ]]; then
    echo "==> clean 실행 중..."
    xcodebuild \
        -project "$PROJECT" \
        -scheme OpenLara \
        -configuration "$CONFIG" \
        -derivedDataPath "$BUILD_DIR" \
        clean
fi

echo "==> 빌드 중..."
xcodebuild \
    -project "$PROJECT" \
    -scheme OpenLara \
    -configuration "$CONFIG" \
    -derivedDataPath "$BUILD_DIR" \
    MACOSX_DEPLOYMENT_TARGET=11.5 \
    build 2>&1 | xcpretty 2>/dev/null || xcodebuild \
        -project "$PROJECT" \
        -scheme OpenLara \
        -configuration "$CONFIG" \
        -derivedDataPath "$BUILD_DIR" \
        MACOSX_DEPLOYMENT_TARGET=11.5 \
        build

APP_PATH=$(find "$BUILD_DIR" -name "OpenLara.app" -maxdepth 6 | head -1)

if [[ -z "$APP_PATH" ]]; then
    echo "[ERROR] OpenLara.app 를 찾을 수 없습니다."
    exit 1
fi

echo "==> 빌드 완료: $APP_PATH"

if [[ $RUN -eq 1 ]]; then
    echo "==> 앱 실행 중..."
    open "$APP_PATH"
fi
