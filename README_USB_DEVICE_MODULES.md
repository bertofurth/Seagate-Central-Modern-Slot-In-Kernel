# README_USB_DEVICE_MODULES.md
This is a very brief and incomplete guide to identifying the Linux
drivers / modules that need to be compiled along with the Linux
kernel in order to support attaching new USB devices to the Seagate 
Central.

By default, the Seagate Central does not support a large range
of connected USB devices. If you wish to connect anything beyond
a basic storage drive then the Seagate Central needs to have
new drivers for the device installed. This means that the Linux
kernel running on the Seagate Central needs to be modified to
include the appropriate components and/or modules to support the
new device.

The best way to create a new customizable kernel for the Seagate
Central is by referring to the other README files in this
**Seagate-Central-Slot-In-v5.x-Kernel** project.

https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel

This is by no means a definitive guide to Linux modules. For more
information please refer to other more complete guides such as

* Kernel Modules - Linux Kernel Documentation

https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html

* Linux Loadable Kernel Module HOWTO

https://tldp.org/HOWTO/Module-HOWTO/

## TLDNR
Figuring out what modules / drivers need to be installed for "embedded"
Linux devices like the Seagate Central is not a well defined process.
In general it's an iterative process and goes as follows.

1) Plug the device into a PC/RPi running a full up-to-date Linux distro.
2) Observe what modules are needed for a the new device.
3) Reconfigure the target kernel with "make menuconfig" and add new modules.
4) Rebuild the kernel and modules. (make uImage, make modules, make modules_install)
5) Check to see what modules were built.
6) If any required modules are missing then GOTO 3) .
7) Install the new kernel and modules on the target (Seagate Central).
8) Plug the new device into the target and see if it's recognized (lsusb / lsmod)
9) If there are "unresolved symbol" or "could not find module" errors (dmesg) then GOTO 3)

Each of these steps is detailed below using the example of 
connecting a USB video camera to the Seagate Central.

An alternative approach which can replace steps 1) and 2) is to use
something like the Linux Kernel Drivers Database.

https://cateee.net/lkddb/

## Plug the device into a PC/RPi running a full up-to-date Linux distro.
Unfortunately there isn't an elegant means to discover all the
drivers / kernel modules required for most USB devices. Many
device manufacturers do not take Linux into account in their
documentation. In addition, many devices are simply "rebadged"
versions of other vendor's equipment. This means that while a
USB device might be labelled on the outside as being a particular
make and model, it may be that from the USB hosts's (computer's)
perspective it's actually recognized as something different.
This in turn means that it's not always obvious what the names
of the required drivers / modules are.

The easiest way to find out the "true" make and model of a USB 
device and the required drivers/modules is to simply plug it in to
a working computer (e.g. a PC or Raspberry PI) running an up to 
date "full" Linux distribution such as OpenSUSE or Debian.

These modern Linux distributions normally have prebuilt modules
and drivers that cover every available device that is supported
by Linux.

If you don't have a Linux machine, then consider building a
"virtual" Linux machine using one of the many free "virtualization 
software" tools available.

## Observe what modules are needed for a the new device.
Once the USB device is connected to the PC, navigate to the 
"/sys/bus/usb/drivers" folder. From there issue the "ls -p" command
to see the directory name that corresponds to the newly plugged in
device.

For example, I have a USB camera that is labelled as a "Cisco" brand
device. The documentation for the device says it's a "Logitech" style
camera however the camera is actually recognized as "Philips webcam"
by the host! For example

    # cd /sys/bus/usb/drivers
    # ls -p
    Philips webcam/ hub/  usb/  usb-storage/  usbfs/  usbhid/  usblp/
     
To find the name of the main driver/Linux module used by this device,
change into the device directory then issue the "readlink module" 
command to see the name of the module being used. For example, for 
the above referenced "Philips webcam" device we see below that the 
main driver/module name is "pwc".

    # cd Philips\ webcam
    # readlink module
    ../../../../module/pwc
    
Now that we have identified the main driver module name, we also need
to identify the names of any modules that the main driver depends on.
This can be done by running "lsmod | grep" command for the module
name as per the following example.

    # lsmod | grep pwc
    pwc                   262144  0
    videobuf2_v4l2        262144  1 pwc
    videobuf2_vmalloc     327680  1 pwc
    videobuf2_common      262144  4 pwc,videobuf2_v4l2,videobuf2_vmalloc,videobuf2_memops
    videodev              393216  3 pwc,videobuf2_v4l2,videobuf2_common

On the left hand side of the "lsmod" command output we can see the
names of secondary modules that the main module depends on. On the
right hand side we see the names of modules that are using the 
module listed on the left. In the above example we see that
"videobuf2_v4l2", "videobuf2_vmalloc", "videobuf2_common" and 
"videodev" are being used by the main driver module "pwc".

To be very thorough, we now need to look at what modules these
secondary modules depend on by running the same style of "lsmod | grep"
command again. Since all of the secondary module names start with the
text "video" we can grep for them all at once as per the following
example.

    # lsmod | grep video
    videobuf2_v4l2        262144  1 pwc
    videobuf2_vmalloc     327680  1 pwc
    videobuf2_memops      327680  1 videobuf2_vmalloc
    videobuf2_common      262144  4 pwc,videobuf2_v4l2,videobuf2_vmalloc,videobuf2_memops
    videodev              393216  3 pwc,videobuf2_v4l2,videobuf2_common
    mc                    327680  3 videobuf2_v4l2,videobuf2_common,videodev

We see that another module "mc" is being used by the secondary modules.

To continue the above example, we run "lsmod | grep" for "mc" and see that 
there are no other modules that "mc" depends on.

    # lsmod | grep mc
    mc                    327680  3 videobuf2_v4l2,videobuf2_common,videodev

Finally, we have the full list of the required modules/drivers to 
support the USB device. In this case it is pwc, videobuf2_v4l2, 
videobuf2_vmalloc, videobuf2_memops, videobuf2_common, videodev and mc .

The next step is to get the text description of each of the above 
identified modules and take note of them. This can be done with the
"modinfo -F description" command applied to each module name. 
Continuing with the example above we have

    # modinfo -F description pwc
    Philips & OEM USB webcam driver
    # modinfo -F description videobuf2_v4l2
    Driver helper framework for Video for Linux 2
    # modinfo -F description videobuf2_vmalloc
    vmalloc memory handling routines for videobuf2
    # modinfo -F description videobuf2_memops
    common memory handling routines for videobuf2
    # modinfo -F description videobuf2_common
    Media buffer core framework
    # modinfo -F description videodev
    Video4Linux2 core driver
    # modinfo -F description mc
    Device node registration for media drivers

## Reconfigure the target kernel with "make menuconfig" and add new modules.
We now need to reconfigure the Linux kernel build parameters to include the
necessary drivers, modules and components that are required to support the 
new device.

The easiest way to reconfigure the Linux kernel build parameters is to use
the "make menuconfig" command which invokes a user friendly, text based menu
where options can be easily enabled and disabled.

Invoke the "make menuconfig" command using the same format as seen in the 
**README_CROSS_COMPILE_KERNEL.md** instructions in this project. That is,
make sure to specify all the required environment variables and the
location of the kernel build directory. For example

    KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make menuconfig

We now need to navigate through the presented configuration menus searching
for the features listed above in the output of the "modinfo -F description"
commands and enable them. The problem is, that the menu options presented
in the "make menuconfig" display are not always exactly the same as the
descriptions output in the "modinfo" command.

Search through options in "make menuconfig" by using the "/" command
and typing the description of what's being searched for. The results
should display the menus that need to be navigated to find each item.

Another more deterministic way of finding out which kernel configuration
options need to be enabled for each module is to search through the 
Linux kernel source code for "Makefile" files containing the kernel module
name followed by ".o" . In these we can see the CONFIG_ option that
needs to be enabled to build the module.

Refering back to the example of the USB camera, we can search for the
"videodev" module as follows from the top of the kernel source code tree.
(Note the space before the module name in the grep command helps eliminate 
false hits.)

    # cd linux-5.14
    # grep -r -F " videodev.o"
    drivers/media/v4l2-core/Makefile:obj-$(CONFIG_VIDEO_V4L2) += videodev.o

We see that for the "videodev" module we need to enable the
CONFIG_MEDIA_SUPPORT option. We can now search for this option using the "/"
search dialog in "make menuconfig" in order to find and enable it.

Note that the hyphen (-) and the underscore (_) are **interchangeable**
in Linux kernel module names so you may have to search for modules names
using both - and _ . For example, for the "videobuf2_common" module

    # grep -r -F " videobuf2_common" *
    (No result)
    # grep -r -F " videobuf2-common" *
    drivers/media/common/videobuf2/Makefile:  videobuf2-common-objs += vb2-trace.o
    drivers/media/common/videobuf2/Makefile:obj-$(CONFIG_VIDEOBUF2_CORE) += videobuf2-common.o

We see that the "videobuf2_common" module corresponds to the 
CONFIG_VIDEOBUF2_CORE option.

In "make menuconfig" navgiate to each required item and use the "space"
key to mark it as either "M" meaning that the item will be built as a kernel
module, or as "*" meaning that the item will be built in to the monolithinc
kernel uImage.

Note that sometimes enabling one required option, will automatically 
enable another. Also note that sometimes, one option depends on another
being enabled before it will even appear as a menu item in "make menuconfig".

Here are the menu items and CONFIG variables that correspond to the
modules listed in our example of a Cisco brand USB Camera.

#### mc : Device node registration for media drivers
CONFIG_MEDIA_SUPPORT
Device Drivers -> Multimedia Support (M)

#### pwc : Philips & OEM USB webcam driver
CONFIG_USB_PWC
Device Drivers -> Multimedia Support -> Media Drivers -> Media USB Adapters (*)

Then under "Media USB Adapters" enable "USB Philips Cameras" (M)

#### videodev : Video4Linux2 core driver
CONFIG_VIDEO_V4L2
Automatically selected by enabling MEDIA_SUPPORT and USB_PWC

#### videobuf2_common : Media buffer core framework
CONFIG_VIDEOBUF2_CORE
Automatically selected by enabling MEDIA_SUPPORT and USB_PWC

#### videobuf2_memops : common memory handling routines for videobuf2
CONFIG_VIDEOBUF2_MEMOPS
Automatically selected by enabling MEDIA_SUPPORT and USB_PWC

#### videobuf2_vmalloc : vmalloc memory handling routines for videobuf2
CONFIG_VIDEOBUF2_VMALLOC
Automatically selected by enabling MEDIA_SUPPORT and USB_PWC

#### videobuf2_v4l2 : Driver helper framework for Video for Linux 2
CONFIG_VIDEOBUF2_V4L2
Automatically selected by enabling  MEDIA_SUPPORT and USB_PWC

      








4) Rebuild the kernel and modules. (make uImage, make modules, make modules_install)
5) Check to see what modules were built.
6) If any required modules are missing then GOTO 3) .
7) Install the new kernel and modules on the target (Seagate Central).
8) Plug the new device into the target and see if it's recognized (lsusb / lsmod)
9) If there are "unresolved symbol" or "could not find module" errors (dmesg) then GOTO 3)

#### Reconfiguring the Linux kernel



configure the kernel, build the kernel and modules, check to see what modules
were built, and then if any modules were missing go back to the start and try again.
    


On a "full" Linux distribution running on a device like a PC or Raspberry PI,
this isn't necessary because most "full" Linux distributions come with 
all the modules and components pre-built and available "out of the box".

This isn't the case for the Seagate Central, because there's no "Seagate
Central" Linux distribution that has automatically pre-built all the
modules and drivers for every device in the world!!

    


Remember, it doesn't hurt to enable the building of extra modules. You can
always delete them later!!

#### Building kernel modules
The following instructions should be read in conjunction with the instructions
in the above mentioned **Seagate-Central-Slot-In-v5.x-Kernel** project.

     
     
     lsusb
     
The out    

It's difficult to provide a guide
