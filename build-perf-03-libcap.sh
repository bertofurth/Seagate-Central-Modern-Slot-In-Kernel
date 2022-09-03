#!/bin/bash
#

# Note that libcap needs to be built "in tree"
# so if you need to re-compile it for any reason
# you'll need to delete the expanded source
# directory then untar the tar archive again.

source build-common
source build-functions
check_source_dir "libcap"
# libcap needs to build "in-tree"
# No configure stage for libcap
export CROSS_COMPILE="$CROSS_COMPILE"

# Need to specify names of gcc and ld for
# building binaries on the local machine
export BUILD_CC="gcc"
export BUILD_LD="ld"
make_it V=1 -C libcap prefix=$PREFIX lib=lib 
install_it V=1 -C libcap prefix=$PREFIX lib=lib
finish_it
