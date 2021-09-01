# Seagate Central Slot In v5.x Kernel
A modern slot in Linux Kernel for the Seagate Central NAS
running stock Seagate Central firmware.

This accompanies the Seagate-Central-Samba project at the
following link 

https://github.com/bertofurth/Seagate-Central-Samba

Installing the samba software at the above project is
a pre-requisite / co-requisite of this project. That is,
you must upgrade the samba service on the Seagate Central
before, or at the same time as upgrading the Linux kernel.

A pre-compiled linux v5.4.X kernel based on the instructions
in this guide is currently available at

BERTO
https://www.dropbox.com/s/wwesnz5cmc9hlcy/seagate-central-samba-4.14.6-15-Aug-2021.tar.gz
BERTO

There are three sets of instructions included in this project.

# INSTRUCTIONS_CROSS_COMPILE_KERNEL.md
Apply necessary patches and cross compile the linux kernel for
Seagate Central from scratch.

# INSTRUCTIONS_INSTALL_KERNEL.md
Manually install the new linux kernel onto the Seagate Central.

# INSTRUCTIONS_INSTALL_WITH_SAMBA.md
Install the kernel as part of a firmware upgrade that includes
system tweaks and the required samba server upgrade.

## Acknowledgements
This project is based on the Seagate Central Firmware GPL
source code as provided at

https://www.seagate.com/gb/en/support/downloads/item/central-gpl-fw-master-dl/

It is also based on the work by KL-Yang in the 
Seagate_central_linux (Single Disk Version) project at

https://github.com/KL-Yang/seagate_central_linux

Finally the 64K memory page size work is largely based on
the work by Gregory Clement as documented at

https://lwn.net/ml/linux-arm-kernel/20200611134914.765827-1-gregory.clement@bootlin.com/

Thank you to all the greater minds than mine that have paved 
the way for this project.

## Pre-compiled kernel download link
BERTO A pre-compiled kernel based on this project and base on linux BERTO BERTO is available at

BERTO  https://www.dropbox.com/s/wwesnz5cmc9hlcy/seagate-central-samba-4.14.6-15-Aug-2021.tar.gz

BERTO See the Installation section below for instructions on how to install
this pre-compiled kernel on a working Seagate Central.

A download link for the required samba software upgrade is available
at the Seagate-Central-Samba project at

https://github.com/bertofurth/Seagate-Central-Samba

## Goals of this project
This project's main goal is to let users seamlessly as possible
install a new and modern version of the linux kernel on a Seagate
Central while still keeping their user Data volume intact and running
the original Seagate supplied tools with as little interruption as
possible.

The hope is that by providing a modern kernel to replace the old 
v2.6 native kernel, users will be able to add new modern software 
services to their Seagate Central without having to go through the
dangerous and tedious process of installing an entirely new operating 
system.

This is different to the project by KL-Yang which provides a kernel
to be run on a Seagate Central to be used as part of installing a
completely new operating system.

The main obstacle this project had to overcome was incorporating 
support for 64K page sizes. This was required because the native disk
format for the user Data partition on a Seagate Central uses 64K pages.
These are only supported natively by linux when the memory pages are
64K or larger.

This project also includes support for the Seagate Central USB port
which the KL-Yang project has not added at the time of writing (check
the project page linked above).

Here is a list of *some* of the new features that can be supported by 
using this upgraded kernel.

### SMP - Symmetric Multiprocessing 
By allowing the unit to make full use of both CPU cores, services can
run faster and more efficiently.

### IPv6
By enabling IPv6 support on the unit all of the privacy and connectivity
advantages provided by an IPv6 enabled network can now be taken advanage
of.

#### Other kinds of USB devices
By using a standard modern linux kernel new kinds of USB devices, such as
a video camera, can be connected to the unit.

#### Security
By using a modern kernel security flaws in older kernel versions are fixed
and are unable to be exploited.

#### New software
By using a modern kernel users are able to install the latest versions of
modern linux software tools that depend on the services and interfaces provided 
by an up to date version of the Linux kernel.



### Samba version 4.x installed on the Seagate Central
The original version of the samba file sharing service loaded on the
Seagate Central (v3.5.16) is not able to work with a modern linux kernel.

This is because the Seagate supplied version of samba makes use of
custom additions to the Seagate supplied linux kernel that are not 
available in standard, modern linux kernels.

For this reason it is important to either first upgrade the samba
software or to perform the upgrade of the kernel and the samba software
simultaneously.

See the Seagate-Central-Samba project at the following link for further
details

https://github.com/bertofurth/Seagate-Central-Samba

It is technically possible to try the new kernel on a Seagate Central 
without upgrading the samba service however it will simply mean that
samba file sharing will not operate! All of the other services on the
Seagate Central, such as the web management interface, the FTP server
and the Twonky DLNA media server, will still work.

### Know how to copy files between your host and the Seagate Central. 
Not only should you know how to transfer files to and from your Seagate
Central NAS and the build host, ideally you'll know how to transfer files 
**even if the samba service is not working**. I would suggest
that if samba is not working to use FTP or SCP which should both still work.

### ssh access to the Seagate Central.
You'll need ssh access to issue commands on the Seagate Central command 
line. 

If you are especially adept with a soldering iron and have the right 
equipment then you could get serial console access but this quite
difficult and is **not required**. There are some very brief details 
of the connections required at

http://seagate-central.blogspot.com/2014/01/blog-post.html

Archive : https://archive.ph/ONi4l

That being said, if you are interested in doing further development of
this kernel then having console access is invaluable for the purposes
of troubeshooting problems during kernel boot.

### su/root access on the Seagate Central.
Make sure that you can establish an ssh session to the Seagate Central
and that you can succesfully issue the **su** command to gain root
priviledges. Note that some later versions of Seagate Central firmware
deliberately disable su access by default.

BERTO The alternative procedure detailed in
**INSTRUCTIONS_FIRMWARE_UPGRADE_METHOD.md** does not require su access
and will in fact automatically re-enable su access as a result of the
procedure.








* NTFS support

The original Seagate Central firmware uses a proprietary driver, called ufsd, to mount USB attached NTFS drives. This allowed the Seagate Central to seamlessly interact with any attached NTFS formatted USB drives.

Unfortunately the v5 slot in kernel is not able to make use of this proprietary driver as it was compiled for use with only the specific kernel version used by the original Seagate Central v2.6 kernel.

Linux does have a read-only version of the NTFS file system however it is reportedly quite slow and as the name suggests it is not able to reliably write data to an NTFS drive.

There is a new much faster and reportedly more reliable NTFS driver called NTFS3 that as of writing is almost ready to be included in the linux kernel. I have gotten this driver to work on the Seagate Central by compiling linux based off the beta testing tree at

https://github.com/Paragon-Software-Group/linux-ntfs3



https://www.digitalcitizen.life/what-is-fat32-why-useful/

Format a drive with FAT32 greater than 32G (up to 8TB)

Main disadvantage of FAT32 is limited individual file size of 4GB


https://www.howtogeek.com/316977/how-to-format-usb-drives-larger-than-32gb-with-fat32-on-windows/





Issues :

USB drives inserted at bootup.

Sometimes when using the v4.x / v5.x "slot in kernel" USB storage devices inserted into the Seagate Central at boot time may not be recognized. Unplugging them and plugging them back in seems to be a work around but this is obviously not convenient.


THIS I WRONG! I DON’T THIKN THIS SCRIPT IS INVOLVED IN MOUTING USB, ONLY THE SHARES VOLUMES

The problem seems to be that the /etc/rcS.d/S20automount script that executes on boot to mount these USB attached drives may execute a few seconds before the inserted USB drives are properly recognized by the system. If you are able to view the boot console log you might see a message saying something like

ENCVOLCRE:List of connected usb drives:
ENCVOLCRE:USB Device/Partition Not Found

There are a two simple ways to address this.

The best way is to simply change the order in which the automount script runs. I've tested moving the script from the default priority 20 to lower priority 97. This seems to resolve the problem when performing my own tests with a variety of USB storage attached at boot. In addition this does not negatively impact operation when using the stock Seagate kernel either.
 
Use the following commands on the Seagate Central CLI to implement this solution

sudo update-rc.d -f automount remove
sudo update-rc.d automount start 97 S .



Another option, which might be less invasive, would be to simply add a small delay before the /etc/rcS.d/S20automount script executes in order to give the system enough time to recognize any attached USB storage.

Below is an example of how to write a small script that lets the system sleep for a few seconds before the automount script runs. Execute the following commands on the Seagate Central command line.


sudo cat << EOF  > /etc/init.d/sleep2
#!/bin/bash
seconds=\`echo \$0 | awk -F "sleep" '{print \$2}'\`
echo Sleeping for \$seconds seconds
sleep \$seconds
echo Finished sleeping for \$seconds seconds
EOF
sudo chmod a+x /etc/init.d/sleep2
ln -s /etc/init.d/sleep2 /etc/rcS.d/S19sleep7


This idea here is that the script will sleep for however many seconds is the last digit of the "S19sleepX" link name that is called.

So in the above example we have created a startup link called

/etc/rcS.d/S19sleep7

So the script will sleep for 7 seconds.

If you decided that this was too long and you want the script to only sleep for 3 seconds you could rename the link as follows

sudo mv /etc/rcS.d/S19sleep7 /etc/rcS.d/S19sleep3


Implementing this solution does not seem to have any negative impact when the unit is running with the stock Seagate kernel except, of course, to make the boot process a few seconds slower.




Didn't put too much effort into "Power Management" aka Suspend/Resume. I'd be surprised if there's anyone suspending and resuming their Seagate Central's to disk. I can see some circumstances where it might be useful to have Suspend and Wake-on-Lan or Wake-on-rtc alarm style functionality but I don't think that was ever part of the original Seagate Central feature set.



Ethernet based Resume on Lan functionality but 




Suggestions :

Completely disable the Tappin remote access agent.

This service has been disabled since April 2018 as per the following notice on Seagate's website

Seagate Central - TappIn Update
https://www.seagate.com/au/en/support/kb/seagate-central-tappin-update-007647en/

Archived Copy
http://archive.today/NE34G

While the service is running in the background it's consuming memory and CPU resources. In addition the bootup process is slowed down by the associated scripts 


An example of one way to disable the TappIn service would be using the following command on the Seagate central command line


sudo update-rc.d -f tappinAgent remove



THIS DOESN’T WORK 

sudo mv /etc/rc5.d/S85tappinAgent /etc/rc5.d/__disabled__S85tappinAgent
sudo mv /etc/rcS.d/S21hpsetup.sh /etc/rcS.d/__disabled__S21hpsetup.sh




Issue : Tracebacks on bootup 

In some cases the system may generate tracebacks similar to

WARNING: CPU: 0 PID: XX at mm/vmalloc.c:XXX vunmap_range_noflush+0xXX/0xXX

WARNING: CPU: 1 PID: XXXX at arch/arm/mm/physaddr.c:XX __virt_to_phys+0xXX/0xXX
kernel: virt_to_phys used for non-linear address: xxxxxxxx (0xb0000000)

These seem to occur due to some of the proprietary Seagate Central software, particularly the "ledmanager" and "networklan" tools. It doesn't seem to impact on the operation of the unit.

#### Issue : Ethernet disconnect handling

When the Ethernet cable is physically disconnected from the unit the system doesn't properly recognize this event. Instead it still believes that the Ethernet interface is "up". After the unit is reconnected to the Ethernet it will simply function as if nothing has happened.

As most dedicated NAS devices are not frequently disconnected from the Ethernet LAN I don't believe this will be a major issue.

The only rare occasion I can envisage where this might be an issue is if someone were deliberately trying to force the Seagate Central to quickly acquire a new DHCP lease by disconnecting then reconnecting the Ethernet LAN cable. 

The workaround is to simply reboot the unit if a new DHCP lease needs to be quickly acquired, or to simply wait until the unit acquires a new DHCP lease after the old one expires.

#### Issue : No red blinking LED status light for lost Ethernet connectivity

When using stock firmware, if a Seagate Central detects that the Ethernet connection is down then the status light on the top of the unit will blink red. This will not occur when using the new kernel. This relates to the issue listed above with Ethernet disconnect handling.

Note that the other LED status states work properly. That is, solid amber after power on, flashing green during the boot process and solid green once the main bootup process is finished. 

Also note that the status LEDs on the Ethernet port itself work fine and will properly indicate the status of the Ethernet port.

#### Issue : Kernel DEBUG and other obscure options (Rare)

In some cases there may be problems when the kernel is compiled with some less frequently used configuration options such as older drivers or those under the "Kernel Hacking" menu of the Linux menuconfig dialog.

I don't believe this is due to anything specific to this project but just a function of the ARM32 architecture combined with various corner cases receiving less focus from the general Linux community in recent times.

Some examples I've encountered in my testing.

* SMP may not work in 4K page size mode with CONFIG_DEBUG_PAGEALLOC_ENABLE_DEFAULT enabled.

* CONFIG_KASAN will cause a hang on boot.

* CONFIG_FORTIFY_SOURCE will cause a hang on boot (Fixed with patch XXXXXXXXXXX)

* CONFIG_GCC_PLUGINS and/or CONFIG_STACKPROTECTOR_PER_TASK - May cause various strange tracebacks and panics similar to

     Kernel panic - not syncing: stack-protector: Kernel stack is corrupted in . . .

* CONFIG_PREEMPT_NONE will stop the Ethernet network from working. 

* CONFIG_AFS_FS, the Andrew File System, will not work with 64K memory pages.

The rule of thumb I have had to follow when encountering "weird" issues is to try compiling the kernel with the least number of debugs and optional features turned on as possible.



### TODO : (But probably not)
Here are some components that I *could* have put some effort into but they weren't part of the original Seagate Central so I didn't bother.

#### Power Management 
In this kernel there's no ability to suspend or to power down one of the CPUs during low CPU usage. It's possible to implement but in my experience a file server needs to be ready to go 100% of the time so it's probably not a feature that would get much use.

#### 64K to 4K block size conversion procedure 
This project supports the 64K block sizes on the Seagate Central data partition by supporting a non standard memory page table size of 64K.

An alternative route would have been to create a guide for converting the user data volume in the Seagate Central from 64K block size to a standard 4K block size. That way none of the 64K PAGE_SIZE work be necessary, however that would have meant the kernel would not be "slot-in" as such.

#### Jumbo Ethernet Frames 
Jumbo Ethernet frames allow devices to transmit frames that contain more than the standard data payload of 1500 bytes. This is in order to be able to reduce the packet header transmission and processing overhead associated with transmitting each packet.

I have never come across any home or small enterprise that uses Jumbo Ethernet frames. One important reason for this is because Jumbo Ethernet frames are not supported over Wifi. Jumbo frames only work over hard Ethernet connections where all the infrastructure between the server and client (switches and routers) support it. In lower end networks, like the ones where a Seagate Central would typically be deployed, it's not certain that all the networking infrastructure would have this capability.

Jumbo frames only have a noticeable impact in contexts where gigabytes of data are *constantly* flowing over a hard ethernet network and where microseconds count.

I see some internet documentation claiming that you can get up to 30% performance improvement with Jumbo Frames enabled however my opinion is that this would only apply in a few rare corner cases. One example might be if a network were running using a shared half duplex 10Mbps Ethernet link rather than the modern standards of full duplex 100Mbps and 1Gbps. It might also apply if you were using quite ancient and underpowered Ethernet switching equipment that was being overwhelmed by a flood of frames, or did not use the modern "Cut Through" switching technique that is enabled in virtually all switching equipment made this century.

####  USB On-The-Go (OTG) support.
I'm confident that the Seagate Central doesn't actually support this however it is functionality that was include in the original Seagate Central Kernel.

USB On-The-Go (OTG) is a mechanism whereby a USB "Device" like a smart phone can act as a USB "Host" so that it can connect to other USB devices like a storage drive or a keyboard. Presumably in the context of the Seagate Central, it would let the NAS act as a "Device" that could connect to a "Host" like a Phone or PC. This way the Phone or PC could directly access the data stored on the NAS via the USB connection. 

I'm confident that the Seagate Central doesn't have this kind of capability because it isn't noted in the Seagate Central documentation and I cannot find any examples online of this kind of functionality being used with a Seagate Central. In addition there doesn't seem to be any method to configure or customize this feature, such as which user's Data folder would be made available over the OTG connection and so forth.

I'm not brave enough to test this functionality by plugging my Seagate Central into my PC or Phone via a reversed polarity male to male USB cable because I'm scared that I'll end up frying something!!

If anyone has actually used this kind of functionality on a Seagate Central then I'd be interested to hear about it and _exactly_ how you got it to work.

#### PCI, SDHCI, XHCI, and other hardware components not used by the Seagate Central

The Cavium CNS3420 CPU, which the Seagate Central is based on, supports a range of different hardware bus types and devices however the Seagate Central does not make use of all of these.

I've focused my efforts on only porting over support for the hardware components that are present on the Seagate Central Single Hard Drive NAS. 


## Final thoughts
I acknowledge that I haven't done things in the most elegant manner with these patches and code. For example there are a lot of "#ifdef" statements introduced into some core kernel source files that get things working, but aren't necessarily very graceful. For these reasons I don't plan to make any effort to submit these patches to the mainline Linux kernel. I also don't make any commitment to maintain this project going forward as future linux versions beyond v5.x are released.

I confess that I haven't always fully comprehended every piece of code I've ported over in order to get this project working. It may be that there are flaws in the original code that I've blindly pulled over. It may be that there are flaws that I've created myself through lack of understanding. As I mentioned, my goal was simply to "get things working".

Since this is the case, I would suggest that you **do not use the products of this project in a mission critical system or in a system that people's health or safety depends on**. 

That being said, I've **not encountered any problems involving data corruption or abrupt loss of connectivity** while testing this kernel myself. 

Hopefully these instructions can serve as a template for upgrading the linux kernel on other Linux based embedded NAS equipment. In particular the 64K page size additions will hopefully prove useful for other armv6 based NAS equipment.

Finally I learned a great deal about Linux, arm32 and embedded systems while writing this guide. Please read these instructions with the understanding that I am still in the process of learning. I trust that this project will help others to learn as well.





