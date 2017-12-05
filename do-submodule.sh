#!/bin/sh
THIS_DIR="$(dirname "$(readlink -f "${0}")")"
if [ -d "${THIS_DIR}/${1}" ]; then
    cd "${THIS_DIR}/${1}"
    if ! [ -f CMakeLists.txt ]; then
        git submodule init
        git submodule update
    fi
fi
