#!/bin/bash
###############################################################
# File:          compile.sh
# Creation Date: 10/11/2022 (DD/MM/YYYY)
# Description:   All-in-one script to update, setup and
#	         build the DSpico firmware.
#
# Author:        pedro-javierf
# Copyright:     LNH team (c) 2022, all rights reserved
################################################################


echo "[>] Configuring project with CMake.."

# Clean previous build/ folders if they exist
rm -rf build/
mkdir build

# Export the SDK Path before running CMAKE
export PICO_SDK_PATH=../pico-sdk

# Specify CMAKE where we want the build tree to be at.
# In our case, the build/ directory.
# The source directory will be . (the current directory,
# where the CMakeLists.txt is located).
cmake -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo -B build/ .

echo "[>] Building FIRMWARE: "

# Go and build the firmware
cd build
make

echo "[>] Build completed. Find the DSpico.uf2 file inside build/ folder"
