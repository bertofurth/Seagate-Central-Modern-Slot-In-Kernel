# README_USB_DEVICE_MODULES.md
This is a very brief guide to identifying the Linux drivers /
modules that need to be compiled along with the Linux kernel 
in order to support attaching new USB devices to the Seagate 
Central. This guide focuses on the example of USB Cameras.

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

For more information about Linux modules please refer to other more
complete guides such as

* Kernel Modules - Linux Kernel Documentation

https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html

* Linux Loadable Kernel Module HOWTO

https://tldp.org/HOWTO/Module-HOWTO/

## TLDNR 1
If you just want USB Video Camera support then all modules for every
USB camera natively supported in Linux can be built by using the included
**config-sc-all-usb-cam-modules.txt** kernel configuration file. Simply
copy this configuration file to the build directory ( obj/.config ) before
compiling your kernel and modules as per the
**README_CROSS_COMPILE_KERNEL.md** instructions in this project.

## TLDNR 2
Hosts with modern Linux distributions come with pre-built modules and 
drivers catering for almost every device available. This is not the case 
for "embedded" systems like the Seagate Central. Figuring out what 
modules and drivers need to be installed on lower end Linux systems like
the Seagate Central is not a well defined process.

The most commonly used method is to simply build and install **all the
modules**, see which ones are used and then delete the ones that are not
used. This can be difficult if storage space is limited.

The method we present in this document tries to be more focused
and attempts to build and install only the bare minimum required modules. 
The process is summarized as follows.

1) Plug the new USB device into a PC / RPi running a full, up-to-date Linux distro.
2) Observe what modules are needed for the new device on the PC.
3) Reconfigure the target kernel with "make menuconfig" and add new modules.
4) Rebuild the kernel and modules. (make uImage, make modules, make modules_install)
5) Confirm that the required modules were built.
6) If any required modules are missing then GOTO 3) .
7) Install the new kernel and modules on the target (Seagate Central).
8) Plug the new device into the target and see if it's recognized (lsusb / lsmod)
9) If there are "unresolved symbol" or "could not find module" errors (dmesg) then GOTO 3)

Each of these steps is detailed below using the example of 
connecting a USB video camera to the Seagate Central.

## Plug the new device into a PC running Linux
Unfortunately, there isn't an elegant means to discover all the
drivers / kernel modules required for most USB devices. 

Many device manufacturers do not take Linux into account in their
documentation. In addition, many devices are simply "rebadged"
versions of other vendor's equipment. This means that while a
USB device might be labelled on the outside as being a particular
make and model, it may be that from the USB host's (computer's)
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

## Observe what modules are needed for the new device.
Once the USB device is connected to the PC, navigate to the 
"/sys/bus/usb/drivers" folder. From there issue the "ls" command
to see the directory name that corresponds to the newly plugged in
device.

For example, I have a USB camera that is labelled on the case as a
"Cisco" brand device. The documentation for the device says it's a 
"Logitech" style camera however the camera is actually recognized 
as "Philips webcam" by the host! 

    # cd /sys/bus/usb/drivers
    # ls 
    hub
    Philips webcam
    usb
    usbfs
     
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
    pwc                    94208  0
    videobuf2_vmalloc      20480  1 pwc
    videobuf2_v4l2         28672  1 pwc
    videobuf2_common       57344  2 videobuf2_v4l2,pwc
    videodev              241664  3 videobuf2_v4l2,pwc,videobuf2_common
    usbcore               311296  6 ehci_pci,snd_usb_audio,snd_usbmidi_lib,pwc,ehci_hcd,uhci_hcd

On the left hand side of the "lsmod" command output we can see the
names of secondary modules that the main module depends on. On the
right hand side we see the names of modules that are using the 
module listed on the left. In the above example we see that
"videobuf2_v4l2", "videobuf2_vmalloc", "videobuf2_common" and 
"videodev" are being used by the main driver module "pwc".

Note that the "usbcore" module shown above is embedded in the
Seagate Central monolithic kernel by default so we don't need
to worry about that module.

To be very thorough, we now need to look at what modules these
secondary modules depend on by running the same style of "lsmod | grep"
command again. Since all of the secondary module names start with the
text "video" we can grep for them all at once as per the following
example.

    # lsmod | grep video
    videobuf2_vmalloc      20480  1 pwc
    videobuf2_memops       20480  1 videobuf2_vmalloc
    videobuf2_v4l2         28672  1 pwc
    videobuf2_common       57344  2 videobuf2_v4l2,pwc
    videodev              241664  3 videobuf2_v4l2,pwc,videobuf2_common
    mc                     57344  4 videodev,snd_usb_audio,videobuf2_v4l2,videobuf2_common
    video                  53248  3 dell_wmi,dell_laptop,i915

We see that another module "mc" is being used by the secondary modules. 
The listed "video" module isn't being used by any of the modules
we're concerned about so we can ignore it.

To continue the above example, we run "lsmod | grep" for "mc" and see that 
there are no other modules that "mc" depends on.

    # lsmod | grep mc
    mc                     57344  4 videodev,snd_usb_audio,videobuf2_v4l2,videobuf2_common

We now have the full list of the required modules/drivers to 
support the new USB device. In this case it is pwc, videobuf2_v4l2, 
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

## Reconfigure the target kernel and add new modules.
We now need to reconfigure the Linux kernel build parameters to include the
necessary drivers, modules and components that are required to support the 
new device.

The easiest way to reconfigure the Linux kernel build parameters is to use
the "make menuconfig" command which invokes a user friendly, text based menu
where options can be easily enabled and disabled.

Run the "make menuconfig" command from the Linux source tree base 
directory using the same command format as seen in the
**README_CROSS_COMPILE_KERNEL.md** instructions. That is, make
sure to specify all the required environment variables and the
location of the kernel build directory. For example

    KBUILD_OUTPUT=../obj ARCH=arm LOADADDR=0x02000000 CROSS_COMPILE=arm-sc-linux-gnueabi- PATH=$HOME/Seagate-Central-Toolchain/cross/tools/bin:$PATH make menuconfig

We now need to navigate through the presented configuration menus 
searching for the modules listed above and enable them. The problem is, 
that the menu options presented in the "make menuconfig" display are
not always exactly the same as the descriptions output in the
"modinfo" command.

A more deterministic way of finding out which kernel configuration
options need to be enabled for each module is to search through the 
Linux kernel source code for "Makefile" files containing the kernel module
name followed by ".o" . In these we can see the CONFIG_ option that
needs to be enabled to build the module.

Referring back to the example of the USB camera, we can search for the
"videodev" module as follows from the top of the kernel source code tree.
(Note the space before the module name in the grep command helps eliminate 
false hits.)

    # cd linux-5.14
    # grep -r -F " videodev.o"
    drivers/media/v4l2-core/Makefile:obj-$(CONFIG_VIDEO_V4L2) += videodev.o

We see that for the "videodev" module we need to enable the
CONFIG_VIDEO_V4L2 option. We can now search for this option using the "/"
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

Search through options in "make menuconfig" by using the "/" command
and searching for the CONFIG_ option corresponding to the desired
modules and components. The results should display a description
that broadly corresponds with the output of the "modinfo" command
and the menus that need to be navigated to find each item.

In "make menuconfig" navigate to each required item and use the "space"
key to mark it as either "M", meaning that the item will be built as a kernel
module, or as "*", meaning that the item will be built in to the monolithic
kernel uImage.

Note that sometimes enabling one required option, will automatically 
enable another. Also note that sometimes, one option depends on another
being enabled before it will even appear as a menu item in "make menuconfig".
This means you might have to enable options out of order. It's not 
always easy to know in advance which options need to be configured
first, so try to "loop" through the list of required options.

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

Automatically selected by enabling CONFIG_MEDIA_SUPPORT and CONFIG_USB_PWC

#### videobuf2_common : Media buffer core framework
CONFIG_VIDEOBUF2_CORE

Automatically selected by enabling CONFIG_MEDIA_SUPPORT and CONFIG_USB_PWC

#### videobuf2_memops : common memory handling routines for videobuf2
CONFIG_VIDEOBUF2_MEMOPS

Automatically selected by enabling CONFIG_MEDIA_SUPPORT and CONFIG_USB_PWC

#### videobuf2_vmalloc : vmalloc memory handling routines for videobuf2
CONFIG_VIDEOBUF2_VMALLOC

Automatically selected by enabling CONFIG_MEDIA_SUPPORT and CONFIG_USB_PWC

#### videobuf2_v4l2 : Driver helper framework for Video for Linux 2
CONFIG_VIDEOBUF2_V4L2

Automatically selected by enabling CONFIG_MEDIA_SUPPORT and CONFIG_USB_PWC

After enabling all the required new options, save the configuration and
exit "make menuconfig".

## Rebuild the kernel and modules. 
Once the new kernel configuration has been saved, rebuild the Linux
kernel for the target (Seagate Central) as per the instructions in
the "Build the kernel" and "Build Linux modules" sections of
**README_CROSS_COMPILE_KERNEL.md**

## Confirm that the required modules were built.
After building the modules as per the instruction in 
**README_CROSS_COMPILE_KERNEL.md**, there should be a module
directory located under the "cross-mod" subdirectory of the
base working directory. Check within that directory to confirm
that all the expected modules have been built. Module files
are named with the ".ko" suffix. For example

    # ls -laR cross-mod/lib/modules/5.14.0-sc/kernel/

You may notice that some extra modules have been built. It doesn't
hurt to have extra modules because if they don't turn out to be
necessary then they can be deleted later.

If some of the expected modules have not been built then go
back to the "make menuconfig" step and try to find out what
extra kernel configuration options need to be selected.

## Install the new kernel and modules on the target (Seagate Central).
See the **README_MANUAL_KERNEL_INSTALLATION.md** document for
details on how to transfer and install the kernel image and
modules to the Seagate Central.

## Plug the new device into the target and see if it's recognized 
Once the Seagate Central has booted using the new kernel image
try to connect the USB device to the system and monitor the
system logs using the "dmesg" command and the "/var/log/syslog"
file. For example,

    [ 183.060000] mc: Linux media interface: v0.10
    [ 183.120000] videodev: Linux video capture interface: v2.00
    [ 183.220000] pwc: Logitech/Cisco VT Camera webcam detected.
    [ 183.430000] pwc: Registered as video0.
    [ 183.450000] input: PWC snapshot button as /devices/platform/ohci-platform.0/usb2/2-1/input/input0
    [ 183.460000] usbcore: registered new interface driver Philips webcam

Ideally the device will be recognized and a corresponding
entry for it will appear in the "/sys/bus/usb/drivers" directory
on the Seagate Central as per when it was connected to the PC.

    NAS:# ls /sys/bus/usb/drivers
    Philips webcam
    hub
    usb
    usb-storage
    usbfs
    usbhid
    usblp

Running the "lsusb" command should show that the device is
physically recognized by the Seagate Central but this does
not mean that the device's drivers have been loaded. Run
the "lsmod" command to show that the required modules have been 
loaded by the system. For example

    NAS:~# lsusb
    Bus 002 Device 002: ID 046d:08b6 Logitech, Inc.
    Bus 002 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub
    Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
    NAS:~# lsmod
    Module                  Size  Used by
    pwc                   262144  0
    videobuf2_v4l2        262144  1 pwc
    videobuf2_vmalloc     327680  1 pwc
    videobuf2_memops      327680  1 videobuf2_vmalloc
    videobuf2_common      262144  4 pwc,videobuf2_v4l2,videobuf2_vmalloc,videobuf2_memops
    videodev              393216  3 pwc,videobuf2_v4l2,videobuf2_common
    mc                    327680  3 videobuf2_v4l2,videobuf2_common,videodev

    
Additionally, most devices will have a corresponding entry
appear in the "/dev" directory. In the example of the USB
camera a new entry called "/dev/video0" will appear.

    NAS:~# ls /dev/video*
    /dev/video0
    
In some cases the device may be recognized and the system
will go through the process of loading modules, however
errors complaining of "unresolved symbols" or
"could not find module". If these errors are seen then it 
indicates that something is missing and the process of
configuring the kernel needs to be restarted to add the
missing components.

If a web search for the specific error message doesn't yield
some guidance, then it may be a matter of searching the Linux
source code for the mentioned symbols and module names to get a
clue as to what components might be missing.

Finally remember that it's ok to enable more modules than
are needed. They can simply be deleted later!

## Note about other types of USB cameras
Although the example in this document focused on finding the
right modules for the rare "pwc" type of USB camera, most other types
of USB cameras can be installed in the same way by just replacing
the "pwc" / "USB Philips Cameras" menu selection with whatever
the correct option under the "Media USB Adapters" menu is.

For example, one of the most common USB camera types is the
"USB Video Class (UVC)" style which uses the "uvcvideo"
module. Simply select the following configuration option
instead 

#### uvcvideo : USB Video Class driver
CONFIG_USB_VIDEO_CLASS

Device Drivers -> Multimedia support -> Media drivers 
  -> Media USB Adapters -> USB Video Class (UVC)

### Further information
The Webcam HOWTO (old but still mostly useful)

https://tldp.org/HOWTO/html_single/Webcam-HOWTO/


