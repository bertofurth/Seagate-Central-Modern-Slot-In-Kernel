#!/bin/sh

# Run this script to download and extract the versions
# of source code this project was tested with. Unless
# otherwise noted these are the latest stable versions
# available at the time of writing.

# Set environment variable SEAGATE_LINUX=1 if you
# want to use Seagate's version of linux headers.

# Based on gcc's download_prerequisites script

linux='https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.1.28.tar.xz'

echo_archives() {
    echo "${linux}"
}

echo_patchnames() {
    echo "../0001-SC-linux-6.1.28.patch"
}

die() {
    echo "error: $@" >&2
    exit 1
}

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
	echo "Extracting $(basename ${ar})"
        tar -xf "$(basename ${ar})" \
	    || die "Cannot extract $(basename ${ar})"
done
unset ar

linuxv=$(basename -s .tar.xz ${linux})
cd $linuxv
for patchname in $(echo_patchnames)
do
    patch -p1 < "${patchname}" ||
	die "Error patching $patchname"
done

echo "Copying new files to Linux source tree"
cp -r ../new-files/* .

# Change back to base directory
cd ..

kernelconfigfile=$(ls -1rv config-sc-*-basic.txt | cut -f1 -d'/' | head -1)
if [ -z $kernelconfigfile ]; then
    echo
    echo Unable to find kernel config file. Exiting.
    exit -1
fi

echo "Copying kernel config file $kernelconfigfile to obj/.config"
cp $kernelconfigfile obj/.config

