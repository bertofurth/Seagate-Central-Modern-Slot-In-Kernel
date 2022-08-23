#!/bin/bash
#
# Build the "perf" tool as found in the linux
# source tree.
#

# Note that building "perf" requires a toolchain that
# "knows" it's own sysroot. That is, when you create the
# toolchain with the Seagate-Central-Toolchain project,
# make sure that WITH_SYSROOT is set to 1 in
# the maketoolchain.sh script.

# Also, make sure to add this line to tools/perf/util/data.h
#
# #include <sys/types.h>
#
# Otherwise you'll get errors similar to
#
# error: unknown type name ‘ssize_t’; did you mean ‘size_t’
#

source build-common
source build-functions

# JOBS is used by make perf to control the number of threads
export JOBS=$J

export SRC=$TOP
check_source_dir "linux"

# Just create the obj directory. Don't change into it
# because we need to be in the linux source root directory
# to compile the profile tool
#change_into_obj_directory
mkdir -p $OBJ/perf

export NO_LIBELF=1
export DESTDIR=$BUILDHOST_DEST/$PREFIX/
make_it -C ./tools/perf O=$OBJ/perf install

# Already installed
# install_it

finish_it
