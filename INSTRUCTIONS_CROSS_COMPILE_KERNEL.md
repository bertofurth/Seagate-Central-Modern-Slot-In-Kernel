# INSTRUCTIONS_CROSS_COMPILE_KERNEL.md



## Prerequisites
### Disk space
BERTO : This procedure will take up to a maximum of about 1GiB of disk space
on the building host. It will only consume about 10?? BERTO megabytes of extra 
storage space on the Seagate Central.

### Time
BERTO : The build components take a total of about 45 minutes to complete on an 
8 core i7 PC. It takes about 6.5 hours on a Raspberry Pi 4B.

### A cross compilation suite on a build host
You can follow the instructions at

https://github.com/bertofurth/Seagate-Central-Toolchain

to generate a cross compilation toolset that will generate binaries,
headers and other data suitable for the Seagate Central.

If you have already gone through the pre-requisite process of compiling
replacement samba software then you should already have this cross
compiling software built and ready to use.

### Required tools
An addition to the above mentioned cross compilation toolset the following
packages or their equivalents may also need to be installed on  the building
system.
BERTO
#### OpenSUSE Tumbleweed - Aug 2021 (zypper add ...)
* zypper install -t pattern devel_basis
* gcc-c++
* unzip
* lbzip2
* bzip2
* libtirpc-devel

#### Debian 10 - Buster (apt-get install ...)
* build-essential
* unzip
* gawk
* curl (or use wget)

BERTO BERTO

## Procedure
### Workspace preparation
If not already done, download the files in this project to a 
new directory on your build machine. 

For example, the following **git** command will download the 
files in this project to a new subdirectory called 
Seagate-Central-Slot-In-v5.x-Kernel

    git clone https://https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel/
    
Alternately, the following **wget** and **unzip** commands will 
download the files in this project to a new subdirectory called
Seagate-Central-Slot-In-v5.x-Kernel-main

    wget https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel/archive/refs/heads/main.zip
    unzip main.zip

Change into this new subdirectory. This will be referred to as 
the base working directory going forward.

     cd Seagate-Central-Slot-In-v5.x-Kernel

### Linux kernel source code download and extraction
The next part of the procedure involves gathering the linux source code
and extracting it.

This procedure was tested using version 5.14 of the linux kernel available
from

https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz

It was also tested using the NTFS3 filesystem which is available
from github at

https://github.com/Paragon-Software-Group/linux-ntfs3

Download the required version of linux into the working directory and
extract it as per the following example.



for each component and installing it into the **src** subdirectory of
the base working directory.

Here we show the versions of software used when generating this guide.
Unless otherwise noted these are the latest stable releases at the
time of writing. Hopefully later versions, or at least those with
the same major version numbers, will still work with this guide.

* gmp-6.2.1
* nettle-3.7.3
* acl-2.3.1
* libtasn1-4.17.0
* gnutls-3.6.16
* openldap-2.3.39 (Should be the same version as Seagate Central)
* samba-4.14.6

Change into the **src** subdirectory of the base working directory
then download the source archives using **wget**, **curl -O** or a 
similar tool as follows. Note that these archives are available from 
a wide variety of sources so if one of the URLs used below does not 
work try to search for another.

    cd src
    wget http://mirrors.kernel.org/gnu/gmp/gmp-6.2.1.tar.xz
    wget http://mirrors.kernel.org/gnu/nettle/nettle-3.7.3.tar.gz
    wget http://download.savannah.gnu.org/releases/acl/acl-2.3.1.tar.xz
    wget http://mirrors.kernel.org/gnu/libtasn1/libtasn1-4.17.0.tar.gz   
    wget https://www.gnupg.org/ftp/gcrypt/gnutls/v3.6/gnutls-3.6.16.tar.xz
    wget https://www.openldap.org/software/download/OpenLDAP/openldap-release/openldap-2.3.39.tgz
    wget https://download.samba.org/pub/samba/samba-4.14.6.tar.gz

Extract each file with the **tar -xf** command.

    tar -xf gmp-6.2.1.tar.xz 
    tar -xf nettle-3.7.3.tar.gz  
    tar -xf acl-2.3.1.tar.xz
    tar -xf libtasn1-4.17.0.tar.gz
    tar -xf gnutls-3.6.16.tar.xz
    tar -xf openldap-2.3.39.tgz
    tar -xf samba-4.14.6.tar.gz


