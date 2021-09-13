# INSTRUCTIONS_CROSS_COMPILE_KERNEL.md
## Summary
This is a guide that describes how to cross compile a replacement
Linux kernel suitable for installation on a Seagate Central NAS device.

Manual installation of the cross compiled kernel is covered by 
**INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md**

This procedure has been tested to work with Linux Kernel version
5.14.

TODO : Retest with v5.15 which should have ntfs3 support.

This procedure has been tested to work on the following building
platforms

* OpenSUSE Tumbleweed (Aug 2021) on x86
* OpenSUSE Tumbleweed (Aug 2021) on Raspberry Pi 4B
* Debian 10 (Buster) on x86

The procedure has been tested to work with make v4.3 and v4.2.1 as well
cross compiler gcc versions 11.2.0, 8.5.0 and 5.5.0.

In general, it is suggested to use the latest stable versions of gcc,
binutils and associated building tools.

The target platform tested was a Seagate Central Single Drive NAS 
running firmware version 2015.0916.0008-F. I'm afraid I don't have
access to the multi-drive / multi-LAN port versions of the Seagate
Central so I'm unable to speak to whether this kernel will work on
those other models.

## Prerequisites
### Disk space
This procedure will take up to a maximum of about 1.3GiB of disk space
on the building host. The generated kernel will only consume about
4MB of storage space on the Seagate Central.

### Time
The kernel build component takes a total of about 8 minutes to complete
on an 8 core i7 PC. It takes about 45 minutes on a Raspberry Pi 4B.

### A cross compilation suite on the build host
You can follow the instructions at

https://github.com/bertofurth/Seagate-Central-Toolchain

to generate a cross compilation toolset that will generate binaries,
headers and other data suitable for building software for the Seagate
Central.

If you have already gone through the pre-requisite process of compiling
replacement samba software for the Seagate Central then you should already
have this cross compiling toolchain built and ready to use.

It is possible to use the generic "arm-none" style cross compiler toolchain
available with many Linux distributions when compiling this kernel 
however since these generic tools are not suitable for building samba or
other userland binaries for the Seagate Central, we suggest that you use
the self generated, purpose built cross compilation toolset instead.

### Required tools
In addition to the above mentioned cross compilation toolset the following
packages or their equivalents may also need to be installed on the building
system.

#### OpenSUSE Tumbleweed - Aug 2021 (zypper add ...)
* zypper install -t pattern devel_basis
* bc
* u-boot-tools
* wget (or use "curl -O")
* git (to download this project)
* cross-arm-none-gcc11 (If no self built cross compiler)

#### Debian 10 - Buster (apt-get install ...)
* build-essential
* bison
* flex
* bc
* u-boot-tools
* libncurses-dev
* git (to download this project)
* gcc-arm-none-eabi (If no self built cross compiler)

### Samba version on the Seagate Central
Although this is not strictly a pre-requisite of this kernel build 
procedure it is worth reemphasizing here that the original samba file
server software on the Seagate Central will not work once the new
kernel built with this procedure is installed on the Seagate Central.

See the Seagate-Central-Samba project at the following link for more
details and instructions on how to upgrade the samba service on the
Seagate Central.

https://github.com/bertofurth/Seagate-Central-Samba

## Build Procedure
### Workspace preparation
If not already done, download the files in this project to a new
directory on your build machine. 

For example, the following **git** command will download the 
files in this project to a new subdirectory called 
Seagate-Central-Slot-In-v5.x-Kernel

    git clone https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel
    
Alternately, the following **wget** and **unzip** commands will 
download the files in this project to a new subdirectory called
Seagate-Central-Slot-In-v5.x-Kernel-main

    wget https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel/archive/refs/heads/main.zip
    unzip main.zip

Change into this new subdirectory. This will be referred to as 
the base working directory going forward.

     cd Seagate-Central-Slot-In-v5.x-Kernel

### Linux kernel source code download and extraction
The next part of the procedure involves gathering the Linux kernel source
code and extracting it.

Download the required version of the Linux kernel into the working directory,
extract it and then change into the newly created directory as per the
following example which uses Linux v5.14.

     wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz
     tar -xf linux-5.14.tar.xz
     cd linux-5.14

TODO : N.B. If ntfs3 functionality is desired, then as of writing there is a
git repository at the following URL that contains pre-release source code
supporting read and write support for NTFS volumes. Hopefully this
functionality will be integrated into the mainline release (v5.15??) soon.

https://github.com/Paragon-Software-Group/linux-ntfs3

TODO : This document will be updated when a stable release of Linux with ntfs3
support becomes available (v5.15?)

### Apply patches
After changing into the Linux source subdirectory, patches need to be applied
to the native Linux kernel source code. The following commands executed from the 
Linux source code base directory will apply the patches

     patch -p1 < ../0001-linux-64K-Page-include.patch
     patch -p1 < ../0002-linux-64K-Page-arm.patch
     patch -p1 < ../0003-linux-64K-Page-mm.patch
     patch -p1 < ../0004-linux-64K-Page-misc.patch
     patch -p1 < ../0005-linux-CNS3XXX-arm.patch
     patch -p1 < ../0006-linux-drivers.patch
     patch -p1 < ../0007-linux-arm32.patch
     
If a version of Linux other than v5.14 is being patched then make careful note
of any failure messages. You may need to manually edit kernel source files where
patches have failed.

TODO : If the version of Linux you are using has support for the NTFS3 file 
system then one more patch *may* need to be applied. This document will be 
updated when a stable release of Linux with NTFS3 support becomes available. 

     patch -p1 < ../0008-linux-optional-ntfs3.patch

### Copy new files
New source files need to be copied into the Linux source tree as follows.

     cp -r ../new-files/* .
     
### Delete obsolete files (optional)
There are a small number of source files that have been replaced and superseded.
These can optionally be deleted but leaving them in place doesn't have any negative
impact.

     rm -f arch/arm/mach-cns3xxx/cns3xxx.h
     rm -f arch/arm/mach-cns3xxx/pm.h

### Kernel configuration file
When building the Linux kernel it is important to use a valid configuration file.
This project includes a kernel configuration file called
**config-seagate-central-v5.14-all-in-one.txt** that will generate a kernel image
containing all the base functionality required for normal operation of the
Seagate Central in one monolithic kernel image without the need for any
Linux modules.

The first thing that needs to be done is for this configuration file to
be copied to the build directory. In these instructions we assume that the
build directory will simply be the "obj" subdirectory of the base working
directory.

From the Linux source code base directory run the command

     cp ../config-seagate-central-v5.14-all-in-one.txt ../obj/.config
     
### make menuconfig     
The following step allows you to customize the kernel configuration by running 
the **make menuconfig** dialog. This dialog presents a user friendly menu driven
interface which allows you to add, remove and modify kernel features.

Even if you do not wish to customize the kernel configuration it is strongly
recommended that this step be run anyway because it automatically reconciles any
differences between the configuration file, your building environment and the
version of Linux being compiled.

There are a number of environment variables that need to be set when running the
"make menuconfig" command . 

#### KBUILD_OUTPUT
This is the location of the build directory and the location of the ".config"
kernel configuration file. 

#### ARCH
This is the name of the cpu architecture we are building the Linux kernel for.
In our case this is always set to "arm", referring to 32 bit arm style CPUs as
used by the Seagate Central.

#### LOADADDR
This is the address in memory where the kernel needs to be copied to when the
Seagate Central boots up. This project has been configured so that it should
always be set to 0x02000000 as per the original Seagate Central kernel.

#### CROSS_COMPILE
This is the prefix of the cross compilation toolset being used. If the toolset
as generated by the Seagate-Central-Toolchain project is being used then by default
this will be "arm-sc-linux-gnueabi-" . If a "generic" arm cross compiler is being
used then this might be something like "arm-none-eabi-". Note that this parameter
will normally have a dash (-) at the end.

#### PATH
If the cross compililation tool binaries are not in the standard path then the 
location of the tools needs to be prepended to the path. This will most likely be
the case when using the tools generated by the Seagate-Central-Toolchain project.
If using the generic arm cross compiler then it probably won't be necessary to set
this variable.

Here is an example of how the make menuconfig command would be run when using
the toolchain as generated by the Seagate-Central-Toolchain project.

     KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make menuconfig
     
When the menuconfig dialog appears you have an opportunity to make changes to the
kernel configuration however in the normal case no changes will need to be made.

Select the "exit" option at the bottom of the menuconfig window. If prompted
"Do you wish to save your configuration" make sure to select "yes" even if you have
not made any changes. As mentioned above this step will make sure that the kernel
configuration file is suitable for your particular build environment.

### Build the kernel
The linux kernel can now be build with the "make uImage" command in order to generate
a kernel suitable for loading on the Seagate Central.

Note that the same environment variables need to be set when running the "make uImage" 
command as when the "make menuconfig" command was run.

Note also that it might be useful to include the "-j[num-cpus]" parameter in the make
command as this will let the compilation process make use of multiple CPU threads
to speed up the build process. In general this j value is set to the number of
available cpu cores on your building system.
    
Here is an example of the "make uImage" command being executed

     KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make -j4 uImage
    
The process should complete with a message similar to the following

      UIMAGE  arch/arm/boot/uImage
    Image Name:   Linux-5.14.0-sc
    Created:      Fri Sep 10 09:58:52 2021
    Image Type:   ARM Linux Kernel Image (uncompressed)
    Data Size:    4129384 Bytes = 4032.60 KiB = 3.94 MiB
    Load Address: 02000000
    Entry Point:  02000000
      Kernel: arch/arm/boot/uImage is ready
      
As per the message, the newly generated uImage kernel file is located
under the build directory at ../obj/arch/arm/boot/uImage . This kernel
image can be installed on the Seagate Central as per the instructions
in **INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md** .

#### Optional - Build Linux modules 
The default kernel configuration file in this project generates a
monolithic Linux kernel containing all the basic functionality required
for the Seagate Central to operate as per the original kernel.

If you wish to reduce the size of the kernel image, or to add new
functionality to the kernel, then you may wish to generate kernel
modules which can be installed alongside the new kernel.

After making the appropriate kernel configuration changes using
a tool such as the "make menuconfig" dialog, build the kernel
modules by issuing the "make modules" command using a command
line similar to the following. Make sure to set all the same environment
variables as used in the previous make commands shown above. For
example

    KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make -j6 modules

Finally, use "make modules_install" to copy all the compiled modules to
a holding directory on the build machine where they can be later copied
to the Seagate Central for installation. Specify the target directory
with the INSTALL_MOD_PATH variable.

In this example the module tree is copied to the "cross-mod" subdirectory
of the base working directory.

    KBUILD_OUTPUT=../obj INSTALL_MOD_PATH=../cross-mod make modules_install
 
## Troubleshooting
Most problems will be due to 

* A needed build system component has not been installed.
* One of the patches failing to be applied to the kernel source tree.
* One of the new files not being copied into the kernel source tree.

If the build component fails then it may be helpful to add the
"-j1 V=1" options when running the make command as per the following example

     .... make -j1 V=1 uImage

These options make sure that only 1 cpu thread is active and that
the make command outputs verbose details of what actions are being
taken. This will make the nature of the issue clearer.

If the kernel build configuration is modified to include new functionality,
then be aware that many older system components have not received as much
testing attention from the Linux community in conjunction with the arm32
platform. It may be that some weird compilation errors could occur that might
require some minor source file modification.
