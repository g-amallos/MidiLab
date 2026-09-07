#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

DIST_DIR="${PROJECT_ROOT}/build/linux"
ASSETS_DIR="${PROJECT_ROOT}/assets"
LIB_DIR="${PROJECT_ROOT}/libs/linux"

TARGET_NAME="midilab"


OS="linux"
OS_TYPE=""
EXE_NAME="${TARGET_NAME}"
DEBUG=""

for arg in "$@"; do
    if [[ "$arg" == "win" || "$arg" == "windows" || "$arg" == "os=win" ]]; then
        OS="win"
        OS_TYPE="os=win"
        EXE_NAME="${TARGET_NAME}.exe"
        DIST_DIR="${PROJECT_ROOT}/build/win"
        LIB_DIR="${PROJECT_ROOT}/libs/win"
    elif [[ "$arg" == "lin" || "$arg" == "linux" || "$arg" == "os=linux" ]]; then
        OS="linux"
        OS_TYPE=""
        EXE_NAME="${TARGET_NAME}"
        DIST_DIR="${PROJECT_ROOT}/build/linux"
        LIB_DIR="${PROJECT_ROOT}/libs/linux"
    elif [[ "$arg" == "debug" ]]; then
        DEBUG="debug=1"
    fi
done

echo "==> Cleaning up previous builds..."
cd "${PROJECT_ROOT}"

#make clean

echo "==> Compiling the project..."
make ${OS_TYPE} ${DEBUG}

# Check if the executable was actually built
if [[ ! -f "${EXE_NAME}" ]]; then
    echo "Error: Executable ${EXE_NAME} not found after compilation."
    exit 1
fi

echo "==> Creating distribution directory: ${DIST_DIR}/"
mkdir -p "${DIST_DIR}"

echo "==> Copying executable to distribution directory..."
cp "${EXE_NAME}" "${DIST_DIR}/"

smart_sync() {
    local src="$1"
    local dest="$2"
    local exclude="$3"

    if command -v rsync &> /dev/null; then
        if [[ -n "$exclude" ]]; then
            rsync -a --delete --exclude="$exclude" "$src/" "$dest/"
        else
            rsync -a --delete "$src/" "$dest/"
        fi
    else
        mkdir -p "$dest"
        cp -ru "$src"/* "$dest/"
    fi
}

echo "==> Smart-syncing assets folder..."
if [[ -d "${ASSETS_DIR}" ]]; then
    mkdir -p "${DIST_DIR}/assets"
    smart_sync "${ASSETS_DIR}" "${DIST_DIR}/assets" "archive"
else
    echo "Warning: '${ASSETS_DIR}' directory not found. Skipping sync."
fi

echo "==> Smart-syncing dynamic libraries..."
if [[ -d "${LIB_DIR}" ]]; then
    LIB_EXT="so"
    if [[ "$OS" == "win" ]]; then
        LIB_EXT="dll"
    fi

    if command -v rsync &> /dev/null; then
        rsync -u --include="/*.${LIB_EXT}" --exclude="/*/" --exclude="*" "${LIB_DIR}/" "${DIST_DIR}/"
    else
        # Fallback using update-only copy with unquoted glob
        shopt -s nullglob
        cp -u "${LIB_DIR}"/*."${LIB_EXT}" "${DIST_DIR}/" 2>/dev/null || true
        shopt -u nullglob
    fi
else
    echo "Warning: '${LIB_DIR}' directory not found. Skipping sync."
fi

echo "==> Creating ZIP archive inside the distribution folder..."
(
    cd "${DIST_DIR}"
    zip -r "${TARGET_NAME}.zip" . -x "${TARGET_NAME}.zip"
)

echo "==> Done! Your distribution files are ready in './${DIST_DIR}'"
ls -la "${DIST_DIR}"

echo "\n\nRun cd ../../build/win/ && ./midilab.exe"