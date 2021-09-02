# INSTRUCTIONS_CROSS_COMPILE_KERNEL.md


BERTO
Tested with linux 5.14



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
replacement samba software for the Seagate Centrel then you should already
have this cross compiling toolchain built and ready to use.

It is possible to use the generic "arm-none" style cross compiler toolchain
available with many linux distributions when compiling linux however since 
these generic tools are not suitable for building samba or other userland 
binaries for the Seagate Central, we suggest that you use the self generated 
cross compilation toolset instead.

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
* wget (or use "curl -O")
* bison
* flex
* libncurses-dev
* bc




* gcc-arm-none-eabi (If no self built cross compiler)


git (optional)




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

    git clone https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel.git
    
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

Download the required version of linux into the working directory, extract
it and then change into the newly created directory as per the following 
example which uses linux v5.14.

     wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz
     tar -xf linux-5.14.tar.xz
     cd linux-5.14

### Apply patches
After changing into the linux source subdirectory patches need to be applied
to the native linux source code. The following commands will apply the patches

     patch -p1 < ../0001-64K-Page-include.patch
     patch -p1 < ../0002-64K-Page-arm.patch
     patch -p1 < ../0003-64K-Page-mm.patch
     patch -p1 < ../0004-64K-Page-misc.patch
     patch -p1 < ../0005-CNS3XXX-arm.patch
     patch -p1 < ../0006-drivers.patch
     patch -p1 < ../0007-arm32.patch
     
If the version of linux you are using has support for the NTFS3 file system then
one more patch *may* need to be applied. This document will be updated when
a stable release of linux with NTFS3 support becomes available. 

      patch -p1 < ../0008-optional-ntfs3.patch

### Copy new files
New files need to be copied into the linux source tree as follows.

     cp -r ../new-files/* .
     
### Delete obselete files (optional)
There are a small number of source files that have been replaced and superceeded.
These can optionally be deleted but leaving them in place doesn't have any negative
impact.

     rm -f arch/arm/mach-cns3xxx/cns3xxx.h
     rm -f arch/arm/mach-cns3xxx/pm.h

### Copy the configuration file to the build directory and customize
When building linux it is imporant to use a valid configuration file. This
project includes a kernel configuration file called
config-seagate-central-v5.14-all-in-one.txt that will generate a kernel image
containing all the base functionality required for normal operation of the
Seagate Central. 

The first thing that needs to be done is for this configuration file to
be copied to the build directory. In these instructions we assume that the
build directory will simply be the "obj" subdirectory of the base working
directory however you can create another directory elsewhere for this purpose.

From the linux source code base directory run the command

     cp ../config-seagate-central-v5.14-all-in-one.txt ../obj/.config
     
The following step allows you to customize the configuration file by running 
the **make menuconfig** dialog. This dialog presents a user friendly menu driven
interface which allows you to add, remove and modify kernel features.

Even if you do not wish to customize the configuration it is strongly recommended
that this step be run anyway because it automatically reconciles any differences
between the configuration file, your building environment and the version of linux
being compiled.

There are a number of environment variables that need to be set when running the
make menuconfig command . 

#### KBUILD_OUTPUT
This is the location of the build directory and the location of the ".config"
kernel configuration file. 

#### ARCH
This is the name of the cpu architecture we are building the linux kernel for.
In our case this is always set to "arm", referring to 32 bit arm style CPUs as
used by the Seagate Central.

#### LOADADDR
This is the address in memory where the kernel needs to be copied to when the
Seagate Central boots up. This project has been configured so that it should
always be set to 0x02000000.

#### CROSS_COMPILE
This is the prefix of the cross compilation toolset being used. If the toolset
as generated by the Seagate-Central-Toolchain project is being used then by default
this will be "arm-sc-linux-gnueabi-" . If a "generic" arm cross compiler is being
used then this might be something like "arm-none-eabi-". Note that this parameter
will normally have a dash (-) at the end.

#### PATH
If the cross compiling tool binaries are not in the standard path then the location
of the tools needs to be prepended to the path. This will most likely be the case
when using the tools generated by the Seagate-Central-Toolchain project. If using
the generic arm cross compiler then it probably won't be necessary to set this
variable.

Here is an example of how the make menuconfig command would be run when using
the toolchain as generated by the Seagate-Central-Toolchain project.

     KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make menuconfig
     
When the menuconfig dialog appears make any changes required then select the "exit"
option at the bottom of the window. 

When prompted "Do you wish to save your configuration" make sure to select "yes" 
even if you have not made any changes. As mentioned above this step will make sure
that the kernel configuration file is suitable for your particular build environment.

### Build the kernel
The linux kernel can now be build with the "make uImage" command in order to generate
a compressed kernel suitable for loading on the Seagate Central.

Note that the same envionment variables need to be set when running the "make uImage" 
command as when the "make menuconfig" command was run.

Note also that it might be useful to include the "-j num-cpus" parameter in the make
command as this will let the compilation process make use of multiple CPU threads
and speed up the build process.

Here is an example of the "make uImage" command being executed

     KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make -j6 uImage
    


There are a number of environment variables that need to be set when running the
make menuconfig command . 

#### LOADADDR=0x02000000


and add
or remove 




the configuration can be changed
however it is possible to modify this configuration with 

within the
one kernel image.





