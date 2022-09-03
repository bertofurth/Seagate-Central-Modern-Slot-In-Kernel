#!/bin/sh

# Run this script to download and extract the versions
# of source code this project was tested with. Unless
# otherwise noted these are the latest stable versions
# available at the time of writing.

# Based on gcc's download_prerequisites script

zlib='https://zlib.net/zlib-1.2.12.tar.xz'
elfutils='https://sourceware.org/elfutils/ftp/0.187/elfutils-0.187.tar.bz2'
libcap='https://mirrors.edge.kernel.org/pub/linux/libs/security/linux-privs/libcap2/libcap-2.65.tar.xz'

echo_archives() {
    echo "${zlib}"
    echo "${elfutils}"
    echo "${libcap}"
}

echo_git() {
    echo
}

die() {
    echo "error: $@" >&2
    exit 1
}

mkdir -p src
cd src

if type wget > /dev/null ; then
    fetch='wget'
else
    if type curl > /dev/null; then
	fetch='curl -LO'
    else
	die "Unable to find wget or curl"
    fi    
fi


for ar in $(echo_archives)
do
	${fetch} "${ar}"    \
		 || die "Cannot download $ar"
        tar -xf "$(basename ${ar})" \
		 || die "Cannot extract $(basename ${ar})"
done
unset ar
