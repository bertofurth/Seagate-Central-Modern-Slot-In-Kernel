#!/bin/bash

# Use version elfutils-0.168 when compiling with
# Seagate central gcc-4.4.1

source ./build-common
source ./build-functions
check_source_dir "elfutils"
change_into_obj_directory
configure_it --prefix=$PREFIX \
	     --bindir=$EXEC_PREFIX/bin \
	     --sbindir=$EXEC_PREFIX/sbin \
	     --host=$ARCH \
	     --disable-debuginfod \
	     --disable-libdebuginfod \
	     --disable-textrelcheck

# For some reason when compiling very old versions of
# perf, such as those in the original seagate central
# linux (v2.6.35) we have to run "make" a few times in a row.
# WE end up the first 2 times with errors similar to
#
# WARNING: TEXTREL found in 'libelf.so'
# WARNING: TEXTREL found in 'libdw.so'
#

#make V=1 -j$J |& tee $LOG/"$NAME"_make_part1a.log
#sleep 5
#make V=1 -j$J |& tee $LOG/"$NAME"_make_part1b.log
#sleep 5

make_it
install_it
finish_it
