# README_CROSS_COMPILE_KERNEL.md
## Summary
This is a guide that describes how to cross compile a replacement
v5.x.x Linux kernel suitable for installation on a Seagate Central NAS 
device.

Manual installation of the cross compiled kernel is covered by 
**README_MANUAL_KERNEL_INSTALLATION.md**

## TLDNR
On a build server with an appropriate cross compilation suite 
installed run the following commands to download and compile
Linux kernel v5.15.81. 

    # Download this project to the build host
    git clone https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel.git
    cd Seagate-Central-Slot-In-v5.x-Kernel
    
    # Download and extract Linux kernel v5.15.78 source code
    # or another close kernel release
    wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.81.tar.xz
    tar -xf linux-5.15.81.tar.xz
    cd linux-5.15.81
     
    # Apply Seagate Central patches to Linux (Make sure to check for FAILED messages)
    patch -p1 < ../0001-SC-linux-5.15.81-arch.patch
    patch -p1 < ../0002-SC-linux-5.15.81-drivers.patch
    patch -p1 < ../0003-SC-linux-5.15.81-fs.patch
    patch -p1 < ../0004-SC-linux-5.15.81-include.patch
    patch -p1 < ../0005-SC-linux-5.15.81-mm.patch
    patch -p1 < ../0006-SC-linux-5.15.81-kernel.patch
    
    # Make sure that there are no .rej files indicating a failed patch
    # If there are, manually fix the problems in any .rej files
    find -name *.rej
    
    # Copy new Seagate Central source files into the Linux source tree
    cp -r ../new-files/* .
    
    # Copy the kernel config file into the build directory
    cp ../config-sc-v5.15.81-basic.txt ../obj/.config
    
    # Add the cross compilation suite directory to the PATH 
    export PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH
    
    # Specify the name of the arm cross compilation suite prefix
    # This could be "arm-none-eabi-" or it could be the
    # Seagate Central specific cross compilation suite
    # if it has been built "arm-sc-linux-gnueabi-"
    export CROSS_COMPILE=arm-none-eabi-    
  
    # Specify the build architecture (arm)
    export ARCH=arm
    
    # Specify the location of the build directory and the .config file
    export KBUILD_OUTPUT=../obj
    
    # Specify the address in memory where the kernel should be loaded to
    export LOADADDR=0x22000000
    
    # Run "make olddefconfig" to ensure kernel config file compatiblility
    make olddefconfig
    
    # Build the kernel (Set appropriate j flag)
    make -j4 uImage
    
    # Optional - If modules have been configured (they aren't by default)
    make -j4 modules
    INSTALL_MOD_PATH=../cross-mod make modules_install
    
The newly generated uImage kernel file is located under the build
directory at ../obj/arch/arm/boot/uImage . This kernel image can be
installed on the Seagate Central as per the instructions in 
**INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md** .

## Tested versions
This procedure has been tested to work building Linux Kernel version
5.15.81. This version has been chosen as it is the latest "Long Term
Support" release available at the time of writing. Other reasonably
close versions of the Linux kernel should also work but may require
some tweaking, especially at the point where the kernel source tree
needs to be patched.

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
those models.

## Prerequisites
### Disk space
This procedure will take up to a maximum of about 1.7GiB of disk space
on the building host. The generated kernel will only consume about
4MB of storage space on the Seagate Central.

### Time
The kernel build component takes a total of about 8 minutes to complete
on an 8 core i7 PC. It takes about 45 minutes on a Raspberry Pi 4B.

### A cross compilation suite on the build host
It is possible to use the generic "arm-none-eabi-" style cross compiler 
toolchain available with many Linux distributions when compiling this kernel.

That being the case, if you also wish to build samba or other userland 
binaries for the Seagate Central, we suggest that you use
the self generated, purpose built cross compilation toolset instead.
You can follow the instructions at

https://github.com/bertofurth/Seagate-Central-Toolchain

to generate a cross compilation toolset that will generate binaries,
headers and other data suitable for building software for the Seagate
Central.

If you have no wish to cross compile anything but the Linux kernel for
the Seagate Central then there's no need to create the Seagate Central
Toolchain and the standard "arm-none-eabi-" style will be sufficient.

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
following example which uses Linux v5.15.81.

     wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.81.tar.xz
     tar -xf linux-5.15.81.tar.xz
     cd linux-5.15.81

### Apply patches
After changing into the Linux source subdirectory, patches need to be applied
to the native Linux kernel source code. The following commands executed from the 
Linux source code base directory will apply the patches. **Please make sure
to execute these commands one at a time and carefully ensure that each
command is successfull before proceeding to the next.**

     patch -p1 < ../0001-SC-linux-5.15.81-arch.patch
     patch -p1 < ../0002-SC-linux-5.15.81-drivers.patch
     patch -p1 < ../0003-SC-linux-5.15.81-fs.patch
     patch -p1 < ../0004-SC-linux-5.15.81-include.patch
     patch -p1 < ../0005-SC-linux-5.15.81-mm.patch
     patch -p1 < ../0006-SC-linux-5.15.81-kernel.patch
     
Make careful note of any "Hunk FAILED" messages. Check for any rejected patches
by running the following command

     find -name *.rej
     
You may need to manually edit kernel source files where patches have failed. This
will most likely happen if you use a version of the linux kernel that is significantly
different to the version that these patches were created using (v5.15.70)

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
**config-sc-v5.15.81-basic.txt** that will generate a kernel image
containing all the base functionality required for normal operation of the
Seagate Central in one monolithic kernel image without the need for any
Linux modules.

This configuration file needs to be copied to the build directory. In these
instructions we assume that the build directory will simply be the "obj"
subdirectory of the base working directory.

From the Linux source code base directory run the command

    mkdir -p ../obj
    cp ../config-sc-v5.15.81-basic.txt ../obj/.config
     
N.B. There is another example configuration file in this project called
**config-sc-v5.15.70-all-usb-cam-modules.txt** that can be copied into place
instead of the above mentioned one if you wish to build modules supporting
USB Video cameras. (See **README_USB_DEVICE_MODULES.md**)

### Set build environment variables
A number of environment variables need to be set correctly in order to
correctly cross compile the Linux kernel.

#### PATH
If the cross compililation tool binaries are not in the standard path then the 
location of the tools needs to be prepended to the path. This will most likely be
the case when using the tools generated by the Seagate-Central-Toolchain project.
If using the generic arm cross compiler then it probably won't be necessary to set
this variable. An example command setting the PATH is as follows

    export PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH
    
#### CROSS_COMPILE
This is the prefix of the cross compilation toolset being used. If a 
"generic" arm cross compiler is being used then this might be 
something like "arm-none-eabi-". If the toolset as generated by the 
Seagate-Central-Toolchain project is being used then by default
this will be "arm-sc-linux-gnueabi-" .  Note that this parameter
will normally have a dash (-) at the end.

    export CROSS_COMPILE=arm-none-eabi-

#### ARCH
This is the name of the cpu architecture we are building the Linux kernel for.
In our case this is always set to "arm", referring to 32 bit arm style CPUs as
used by the Seagate Central.
    
    export ARCH=arm

#### KBUILD_OUTPUT
This is the location of the build directory and the location of the ".config"
kernel configuration file. (Make sure the specified directory exists and
contains the ".config" kernel configuration file)

    export KBUILD_OUTPUT=../obj

#### LOADADDR
This is the address in memory where the kernel needs to be copied to when the
Seagate Central boots up. This project has been configured so that it should
always be set to 0x22000000.

    export LOADADDR=0x22000000
    
### make olddefconfig
The following step makes sure that your kernel configuration is up to date
and compatible with the kernel version being built. It automatically reconciles 
any differences between the configuration file, your building environment and
the version of Linux being compiled.

Make sure that the environment variables discussed in the previous section are
correctly set before executing this command.

    make olddefconfig
     
### Optional - make menuconfig
If desired the kernel configuration can be customized by running the 
**make menuconfig** dialog. This dialog presents a user friendly menu driven
interface which allows you to add, remove and modify kernel features.


    make menuconfig
     
Once you have finished making any required changes, select the "exit" option at
the bottom of the menuconfig window. If prompted "Do you wish to save your 
configuration" make sure to select "yes". 

### Build the kernel
The linux kernel can now be build with the "make uImage" command in order to generate
a kernel suitable for loading on the Seagate Central.

Note also that it might be useful to include the "-j[num-cpus]" parameter in the make
command as this will let the compilation process make use of multiple CPU threads
to speed up the build process. In general this j value is set to the number of
available cpu cores on your building system.
    
Here is an example of the "make uImage" command being executed

    make -j4 uImage
    
The process should complete with a message similar to the following

     Image Name:   Linux-5.15.81-sc
     Created:      Sat Dec  3 16:27:06 2022
     Image Type:   ARM Linux Kernel Image (uncompressed)
     Data Size:    4312760 Bytes = 4211.68 KiB = 4.11 MiB
     Load Address: 22000000
     Entry Point:  22000000
       Kernel: arch/arm/boot/uImage is ready
      
As per the message, the newly generated uImage kernel file is located
under the build directory at ../obj/arch/arm/boot/uImage . This kernel
image can be installed on the Seagate Central as per the instructions
in **INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md** .

#### Optional - Build Linux modules 
The default kernel configuration file in this project generates a
monolithic Linux kernel containing all the basic functionality required
for the Seagate Central to operate as per the original kernel.

If you want to reduce the size of the kernel image, or to add new
functionality to the kernel, then you may wish to generate kernel
modules which can be installed alongside the new kernel. See the
**README_USB_DEVICE_MODULES.md** file in this project for 
instructions on adding module support for USB devices.

After making the appropriate kernel configuration changes using
a tool such as the "make menuconfig" dialog, build the kernel
modules by issuing the "make modules" command using a command
line similar to the following. Make sure that all of the same environment
variables are set as used in the previous make commands shown above. For
example

    make -j6 modules

Finally, use "make modules_install" to copy all the compiled modules to
a holding directory on the build machine where they can be later copied
to the Seagate Central for installation. Specify the target directory
with the INSTALL_MOD_PATH variable.

In this example the module tree is copied to the "cross-mod" subdirectory
of the base working directory.

    INSTALL_MOD_PATH=../cross-mod make modules_install
 
## Troubleshooting
Most problems will be due to 

* A needed build system component has not been installed.
* One of the patches failing to be applied to the kernel source tree.
* One of the new files not being copied into the kernel source tree.

If the build fails then it may be helpful to add the "-j1 V=1" options 
to the "make" commnd line as per the following example

     .... make -j1 V=1 uImage

These options make sure that only 1 cpu thread is active and that
the make command outputs verbose details of what actions are being
taken. This will make the nature of the issue clearer.

If the kernel build configuration is modified to include new 
functionality, then be aware that many older system components have 
not received as much testing attention from the Linux community in 
conjunction with the arm32 platform. It may be that some weird 
compilation errors could occur that might require some minor 
source file modification.
