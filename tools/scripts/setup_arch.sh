#!/bin/bash -e
#
# Bareflank Hypervisor
#
# Copyright (C) 2015 Assured Information Security, Inc.
# Author: Rian Quinn        <quinnr@ainfosec.com>
# Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
# Author: Kyle J. Temkin    <temkink@ainfosec.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

# ------------------------------------------------------------------------------
# Checks
# ------------------------------------------------------------------------------

if [[ ! -f /etc/os-release ]] || ! (grep -q "ID.*=arch" /etc/os-release); then
    echo "This script must be used with Arch (or an Arch-derived) distribution!"
fi


if [[ ! -d "bfelf_loader" ]]; then
    echo "This script must be run from bareflank root directory"
    exit 1
fi

# ------------------------------------------------------------------------------
# Help
# ------------------------------------------------------------------------------

option_help() {
    echo -e "Usage: $0 [OPTION]"
    echo -e "Sets up the system to compile / use Bareflank"
    echo -e ""
    echo -e "       -h, --help                       show this help menu"
    echo -e "       -l, --local_compilers            setup local cross compilers"
    echo -e "       -n, --no-configure               skip the configure step"
    echo -e "       -g, --compiler <dirname>         directory of cross compiler"
    echo -e "       -o, --out_of_tree <dirname>      setup out of tree build"
    echo -e ""
}

# ------------------------------------------------------------------------------
# Functions
# ------------------------------------------------------------------------------

install_common_packages() {
    sudo pacman -Sy --noconfirm --needed \
        base-devel linux-headers gmp libmpc flex bison nasm \
        texinfo cmake docker
}

prepare_docker() {
    sudo usermod -a -G docker $(whoami)
    sudo systemctl enable docker
}

# ------------------------------------------------------------------------------
# Arguments
# ------------------------------------------------------------------------------

while [[ $# -ne 0 ]]; do

    if [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
        option_help
        exit 0
    fi

    if [[ $1 == "-l" ]] || [[ $1 == "--local_compilers" ]]; then
        local="true"
    fi

    if [[ $1 == "-g" ]] || [[ $1 == "--compiler" ]]; then
        shift
        compiler="-g $1"
    fi

    if [[ $1 == "-n" ]] || [[ $1 == "--no-configure" ]]; then
        noconfigure="true"
    fi

    if [[ $1 == "-o" ]] || [[ $1 == "--out_of_tree" ]]; then
        shift
        out_of_tree="true"
        build_dir=$1
        hypervisor_dir=$PWD
    fi

    shift

done

# ------------------------------------------------------------------------------
# Setup System
# ------------------------------------------------------------------------------

# Arch doesn't have versions, strictly, so we won't check.
install_common_packages
prepare_docker

# ------------------------------------------------------------------------------
# Setup Build Environment
# ------------------------------------------------------------------------------

if [[ ! $noconfigure == "true" ]]; then
    if [[ $out_of_tree == "true" ]]; then
        mkdir -p $build_dir
        pushd $build_dir
        $hypervisor_dir/configure
        popd
    else
        ./configure $compiler
    fi
fi

if [[ $local == "true" ]]; then
    CROSS_COMPILER=gcc_520 ./tools/scripts/create_cross_compiler.sh
fi

# ------------------------------------------------------------------------------
# Done
# ------------------------------------------------------------------------------

if ! (sudo systemctl start docker); then
  echo ""
  echo "WARNING: If docker did not start up, you should reboot your machine to "
  echo "         allow docker to properly initialize before compiling."
  echo ""
fi


if ! (groups | grep -q docker); then
  echo ""
  echo "WARNING: Your user does not appear to be in the docker group;"
  echo "         you'll want to use newgrp prior to building."
  echo ""
fi

if [[ $out_of_tree == "true" ]]; then
    echo "To build, run:"
    echo "    cd $build_dir"
    echo "    make -j<# cores>"
    echo ""
fi
