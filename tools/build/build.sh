#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 2. Go up two levels safely to reach the project root
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# --- Configuration (Relative to Project Root) ---
DIST_DIR="${PROJECT_ROOT}/dist"
ASSETS_DIR="${PROJECT_ROOT}/assets"
TARGET_NAME="midilab"

# Determine the executable name based on the OS variable passed to Make
# (Defaulting to Linux 'midilab', or 'midilab.exe' if os=win is provided to this script)
OS_TYPE=""
EXE_NAME="${TARGET_NAME}"

if [[ "$1" == "os=win" ]]; then
    OS_TYPE="os=win"
    EXE_NAME="${TARGET_NAME}.exe"
fi

echo "==> Cleaning up previous builds..."
cd "${PROJECT_ROOT}"

make clean
rm -rf "${DIST_DIR}"

echo "==> Compiling the project..."
make ${OS_TYPE}

# Check if the executable was actually built
if [[ ! -f "${EXE_NAME}" ]]; then
    echo "Error: Executable ${EXE_NAME} not found after compilation."
    exit 1
fi

echo "==> Creating distribution directory: ${DIST_DIR}/"
mkdir -p "${DIST_DIR}"

echo "==> Moving executable to distribution directory..."
mv "${EXE_NAME}" "${DIST_DIR}/"

echo "==> Copying assets folder..."
if [[ -d "${ASSETS_DIR}" ]]; then
    cp -r "${ASSETS_DIR}" "${DIST_DIR}/"
else
    echo "Warning: '${ASSETS_DIR}' directory not found. Skipping copy."
fi


echo "==> Cleaning up unwanted asset subdirectories..."
# Delete the specific subdirectory from the copied assets folder
if [[ -d "${DIST_DIR}/assets/archive" ]]; then
    rm -rf "${DIST_DIR}/assets/archive"
fi

echo "==> Creating ZIP archive inside the distribution folder..."
(
    cd "${DIST_DIR}"
    # Use literal names here so zip captures them relative to the current directory (dist/)
    if [[ -d "assets" ]]; then
        zip -r "${TARGET_NAME}.zip" "${EXE_NAME}" "assets"
    else
        zip -r "${TARGET_NAME}.zip" "${EXE_NAME}"
    fi
)

echo "==> Done! Your distribution files are ready in './${DIST_DIR}'"
ls -la "${DIST_DIR}"