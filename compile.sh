#!/usr/bin/env bash
###############################################################
# File:          compile.sh
# Creation Date: 10/11/2022 (DD/MM/YYYY)
# Description:   All-in-one script to update, setup and
#	         build the DSpico firmware.
#
# Author:        pedro-javierf
# Copyright:     LNH team (c) 2022, all rights reserved
################################################################

set -euo pipefail

if command -v nproc >/dev/null 2>&1; then
	NPROC="$(nproc)"
else
	# macOS doesn't have nproc(1). using getconf here is also more general than
	# using sysctl
	NPROC="$(getconf _NPROCESSORS_ONLN)"
fi

# change directory to the one containing the source code project
cd "$(dirname "$(realpath "$0")")"

# check available CMake version
echo "[>] Configuring project with CMake.."

# Clean previous build/ folders if they exist
rm -rf build/ || true
mkdir -p build

# Specify CMAKE where we want the build tree to be at.
# In our case, the build/ directory.
# The source directory will be . (the current directory,
# where the CMakeLists.txt is located).
PICO_SDK_PATH=../pico-sdk \
CMAKE_POLICY_VERSION_MINIMUM=3.5 \
	cmake -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo \
		-B build/ .

echo "[>] Building FIRMWARE: "

# Go and build the firmware
PICO_SDK_PATH=../pico-sdk \
CMAKE_POLICY_VERSION_MINIMUM=3.5 \
CMAKE_BUILD_PARALLEL_LEVEL="${NPROC}" \
	cmake --build build

echo "[>] Build completed. Find the DSpico.uf2 file inside build/ folder"
