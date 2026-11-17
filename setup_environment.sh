#!/bin/bash
###############################################################
# File:          setup_environment.sh
# Creation Date: 10/11/2022 (DD/MM/YYYY)
# Description:   All-in-one script to update and setup 
#                the DSpico firmware repository.
#
# Author:        pedro-javierf
# Copyright:     LNH team (c) 2022, all rights reserved
################################################################

# TODO: See if packages need to be installed

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac


# if linux or CYGWIN
if [[ "$machine" == "Linux" ]]
then
    echo "Detected Linux OS. Installing needed libs."
    sudo apt update
    sudo apt install cmake gcc-arm-none-eabi build-essential git
fi

if [[ "$machine" == "Cygwin" ]]
then
    echo "Detected Cygwin. Installing needed libs."
    sudo apt update
    sudo apt install cmake gcc-arm-none-eabi build-essential git
fi

  
# if MacOS
if [[ "$machine" == "Mac" ]]
then
    # Install cmake
    brew install cmake

    # Install the arm eabi toolchain
    brew tap ArmMbed/homebrew-formulae
    brew install arm-none-eabi-gcc

    # The equivalent to build-essential on linux, you probably already have this.
    xcode-select --install
fi

echo "[>] Updating pico-sdk submodule.."
git submodule update --init -- pico-sdk/

echo "[>] Updating pico-sdk dependencies.."
cd pico-sdk/
git submodule update --init

