# Seagate Central Slot In v5.x Kernel
A modern slot in Linux Kernel for the Seagate Central NAS
running stock Seagate Central firmware.

This accompanies the Seagate-Central-Samba project at the
following link 

https://github.com/bertofurth/Seagate-Central-Samba

Installing the samba software at the above project is
a strongly recommended pre-requisite / co-requisite of this 
project. If you want Windows style samba network file sharing
to work then you must upgrade the samba service on the Seagate 
Central before, or at the same time as upgrading the Linux kernel.

A pre-compiled linux v5.4.X kernel based on the instructions
in this guide is currently available at

BERTO
https://www.dropbox.com/s/wwesnz5cmc9hlcy/seagate-cxxxxxxxxxxx.tar.gz
BERTO

There are three sets of instructions included in this project.

# INSTRUCTIONS_CROSS_COMPILE_KERNEL.md
Cross compile the new v5.x linux kernel for Seagate Central from scratch.

# INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md
Manually install the new linux kernel onto the Seagate Central.

# INSTRUCTIONS_INSTALL_WITH_SAMBA_FIRMWARE_UPGRADE.md
Install the kernel as part of an easy web management based firmware
upgrade process that also upgrades the samba server at the same time.

## Details
This project's main goal is to let users seamlessly as possible
install a new and modern version of the linux kernel on a Seagate
Central while still keeping their user Data volume intact and using
as much of the the original Seagate supplied facilities as possible
and with as little interruption as possible.

The hope is that by providing a modern kernel to replace the old 
v2.6 native kernel, users will be able to add new modern software 
services to their Seagate Central without having to go through the
dangerous and tedious process of installing an entirely new operating 
system.

This is different to the Seagate_central_linux (Single Disk Version)
project by KL-Yang (see Acknowledgements) which provides a kernel to
be run on a Seagate Central to be used as part of a completely new
operating system and without the original Seagate supplied tools.

The main obstacle this project had to overcome was incorporating 
support for 64K memory page sizes. This was required because the native
disk format for the user Data partition on a Seagate Central uses 64K
pages. These are only supported natively by linux when the memory pages
are 64K or larger.

This project also includes support for a number of other notable features
such as 
* USB port - Allows connection of external drives and other devices.
* Symmetric Multiprocessing (SMP) - More efficient and faster processing.
* IPv6 - The new standard for internet networking.
* exFAT - New support for exFAT formatted external drives.
* Real time clock (RTC)
* Access to u-boot environment in flash
* Fixed Ethernet mac-address stored in flash
* Control of status indicator LED
* TODO : NTFS (see below)

## Warning 
**Performing modifications of this kind on the Seagate Central is not 
without risk. Making the changes suggested in these instructions will
likely void any warranty and in rare circumstances may lead to the
device becoming unusable or damaged.**

**Do not use the products of this project in a mission critical system
or in a system that people's health or safety depends on.**

It is worth noting that during the testing and development of this
procedure I never encountered any problems involving data corruption 
or abrupt loss of connectivity.

In addition, I have never come close to "bricking" any Seagate Central!

The Seagate Central boot loader (u-boot) has a feature where it
automatically reverts to the previous version of firmware if it finds
it is unable to bootup the system after 4 consecutive attempts. This
normally overcomes any kind of cataclysmic software induced failure.

In the absolute worst case where a Seagate Central were rendered totally
inoperable there is always the option of physically opening the Seagate
Central, removing the hard drive and mounting it on a different machine
in order to resurrect it, and if necessary, retrieve any user data.

## Caveats
There are some aspects of the Seagate Central that make it difficult
to create a truly "plug-in" kernel replacement. They are listed in
detail here in approximate order of importance. Only the first one
involving the samba service is really necessary to pay strict
attention to.

### samba (SOP / AMP)
The one major aspect that makes this kernel not quite "plug-in" as such
is that the original native version of the samba file serving software on
the Seagate Central will not work with the new kernel. This section will
deal with why this is and why it is necessary, and in fact desirable, 
to upgrade the samba software either before, or at the same time as
upgrading the kernel. 

Samba is the software that allows windows based clients to access the
Seagate Central via the "Network" folder in Windows Explorer. It uses
a network sharing protocol called SMB to perform this task. 

It is technically possible to try the new kernel on a Seagate Central 
without upgrading the samba service however it will simply mean that
samba file sharing will not operate! All of the other services on the
Seagate Central, such as the web management interface, the ssh/sftp/ftp
servers and the Twonky DLNA media server, will still work.

The original Seagate supplied version of samba (v3.5.16) uses an old
and insecure version of SMB (v1.0) which has critical security 
vulnerabilities that make it almost unuseable with modern security
conscious clients such as Windows 10 or later. If you are still
interested in using a Seagate Central in a modern network while still
using Windows style network file sharing then it is virtually mandatory
that you perform an upgrade of the samba service to a more modern and
secure version.

The Seagate Central is based on a Cavium CNS3420 CPU which has 2 CPU 
cores. In stock Seagate Central firmware, the first CPU core is available
for normal linux processes and the second is reserved exclusively for the 
samba file server. In other words, the second CPU cannot be used by any 
"normal" linux processes. In Seagate Central software this scheme is
referred to as the "Cavium SMP Offloading Procession" (SOP), or in
general terms "Asymmetric Multi Processing" (AMP).

The original samba v3.5.16 software on the Seagate Central has custom 
modifications that allow it to make use of the second CPU core operating
in SOP / AMP mode. This means that in the unlikely event that the first 
Seagate Central CPU is overwhelmed by some non file-sharing task, the file 
serving functionality will not be slowed down as it will be using the
second CPU.

Unfortunately, normal modern samba software does not make use of AMP
mode. This is because AMP is rarely implemented in modern linux systems.
Most modern linux systems use "Symmetrical Multi Processing" (SMP) which 
allows **all** linux processes to make use of **any** CPU.

Since AMP is not implemented in the linux kernel generated by this guide,
or in fact by any other modern linux kernel that I know of, the original
Seagate supplied samba software will not work with this new kernel.

See the the Seagate-Central-Samba project at the following link for
more details, explanation, and an easy to follow guide in how to upgrade
the samba software on the Seagate Central.

https://github.com/bertofurth/Seagate-Central-Samba

### NTFS external drive "write" support (Soon to be fixed in kernel v5.15??)
Initially, this project will only support "read" access to external USB
attached NTFS drives. This is expected to be resolved with the advent of
the paragon read/write capable ntfs3 driver due to be released in Linux
kernel v5.15 or soon after.

The original Seagate Central firmware uses a proprietary driver module, 
called ufsd, to mount USB attached NTFS drives. This allowed the Seagate
Central to seamlessly interact with any attached NTFS formatted USB drives.

Unfortunately the v5.x slot in kernel is not able to make use of this
proprietary driver as it was compiled for use with only the specific kernel
version used by the original Seagate Central v2.6 kernel.

Linux does have a read-only version of the NTFS file system, which is enabled
by default using the procedure in this guide, however it is reportedly quite 
slow and is only able to access an NTFS formatted drive in read-only mode.

There is a soon to be released new, much faster and reportedly more reliable
read and write capable NTFS driver called ntfs3 by paragon software. As of
writing is almost ready to be included in the linux kernel. I have sucessfully
tested a beta version of this driver on the Seagate Central by using the
software available at

https://github.com/Paragon-Software-Group/linux-ntfs3

Regardless, my suggestion is to always format externally attached drives as
FAT32, rather than NTFS. This ensures that your drive will be compatible with 
the largest range of devices as possible. Please refer to the following links
that go into much more detail.

* How-To Geek : What File System Should I Use For My USB Drive?
https://www.howtogeek.com/73178/what-file-system-should-i-use-for-my-usb-drive/

* Digital Citizen : What is FAT32 and why is it useful?
https://www.digitalcitizen.life/what-is-fat32-why-useful/

* Format a drive with FAT32 greater than 32G (up to 8TB)
https://www.howtogeek.com/316977/how-to-format-usb-drives-larger-than-32gb-with-fat32-on-windows/

#### Ethernet disconnect handling
While running the new kernel, when the Ethernet cable is physically 
disconnected from the unit the system doesn't properly recognize this
event. Instead it still believes that the Ethernet interface is "up".
After the unit is reconnected to the Ethernet it will simply function
as if nothing has happened.

As most dedicated NAS devices are not frequently disconnected from the
Ethernet LAN I don't believe this will be a major issue.

The only rare occasion I can envisage where this might be an issue is
if someone were deliberately trying to force the Seagate Central to 
quickly acquire a new DHCP lease by disconnecting then reconnecting the
Ethernet LAN cable. 

The workaround is to simply reboot the unit if a new DHCP lease needs
to be quickly acquired, or to simply wait until the unit acquires a
new DHCP lease after the old one expires.

#### No red blinking LED status light for lost Ethernet connectivity
When using native Seagate supplied stock firmware, if a Seagate Central
detects that the Ethernet connection is down then the status light on
the top of the unit will blink red.

This will not occur when using the new kernel. This relates to the
issue listed above with Ethernet disconnect handling.

Note that the other LED status states work properly. That is, solid
amber after power on, flashing green during the boot process and solid
green once the main bootup process is finished. 

Also note that the status LEDs on the Ethernet port itself work fine
and will properly indicate the status of the Ethernet port.

#### Kernel DEBUG and other obscure options (Rare)
In some cases there may be problems when the kernel is compiled using a
non default configuration using some less frequently used options such
as older kernel drivers or those under the "Kernel Hacking" menu of
the Linux menuconfig dialog.

I don't believe this is due to anything specific to this project but 
just a function of the ARM32 architecture combined with various corner
cases receiving less focus from the general Linux community in recent 
times.

Some examples I've encountered in my testing.

* SMP may not work in 4K page size mode with CONFIG_DEBUG_PAGEALLOC_ENABLE_DEFAULT enabled.

* CONFIG_KASAN will cause a hang on boot.

* CONFIG_FORTIFY_SOURCE may cause a hang on boot (Fixed with patch 000)

* CONFIG_GCC_PLUGINS and/or CONFIG_STACKPROTECTOR_PER_TASK - May cause various strange tracebacks and panics similar to

     Kernel panic - not syncing: stack-protector: Kernel stack is corrupted in . . .

* CONFIG_PREEMPT_NONE will stop the Ethernet network from working. 

* CONFIG_AFS_FS, the Andrew File System, will not work with 64K memory pages.

The rule of thumb I have had to follow when encountering "weird" 
issues is to try compiling the kernel with the least number of debugs
and optional features turned on as possible.

#### Unusual tracebacks and errors on bootup 
Due to some minor incompatibilities between the new kernel and some of 
the original software on the Seagate Central, a careful examination of 
the bootup logs may reveal some error messages and tracebacks. 

These messages do not impact on the main functionality of the unit.

Some examples of these messages include

    WARNING: CPU: X PID: XX at mm/vmalloc.c:XXX vunmap_range_noflush+0xXX/0xXX

    WARNING: CPU: X PID: XXXX at arch/arm/mm/physaddr.c:XX __virt_to_phys+0xXX/0xXX kernel: virt_to_phys used for non-linear address: XXXXXXXX (0xXXXXXXXX)
    
    udevd[XXX]: BUS= will be removed in a future udev version, please use SUBSYSTEM= to match the event device, or SUBSYSTEMS= to match a parent device, in /etc/udev/rules.d/98-custom-usb.rules:10
    
    udevd (XXX): /proc/XXX/oom_adj is deprecated, please use /proc/XXX/oom_score_adj instead.
    
These warnings seem to occur due to some of the proprietary Seagate 
Central software, particularly the "ledmanager" and "networklan" tools.

If the entire procedure, including the optional components of the kernel
installation instructions are followed then these warning messages can
be eliminated.

### TODO : (But probably not)
Here are some components that I *could* have put some effort into but 
they weren't part of the original Seagate Central so I didn't bother.

#### Power Management 
In this kernel there's no ability to suspend or to power down one of
the CPUs during low CPU usage. It's possible to implement but in my
experience a file server needs to be ready to go 100% of the time so
it's probably not a feature that would get much use.

#### 64K to 4K partition block size conversion procedure 
This project supports the 64K block sizes on the Seagate Central data
partition by supporting a non standard memory page table size of 64K.

An alternative route would have been to create a guide for converting
the user data volume in the Seagate Central from 64K block size to a 
standard 4K block size. That way none of the 64K PAGE_SIZE work be 
necessary, however that would have meant the kernel would not be 
"slot-in" as such.

#### Jumbo Ethernet Frames 
Jumbo Ethernet frames allow devices to transmit frames that contain 
more than the standard data payload of 1500 bytes. This is in order 
to be able to reduce the packet header transmission and processing 
overhead associated with transmitting each packet.

I have never come across any home or small enterprise that uses Jumbo
Ethernet frames. One important reason for this is because Jumbo Ethernet
frames are not supported over Wifi. Jumbo frames only work over hard 
Ethernet connections where *all* the infrastructure between the server 
and client (switches and routers) support it. In lower end networks, 
like the ones where a Seagate Central would typically be deployed, it's 
not certain that all the networking infrastructure would have this
capability.

Jumbo frames only have a noticeable impact in contexts where gigabytes 
of data are *constantly* flowing over a hard ethernet network and 
where microseconds count.

I see some internet documentation claiming that you can get up to 
30% performance improvement with Jumbo Frames enabled however my opinion
is that a benefit would only be noticed in a few rare corner cases.

One example might be if a network were running using a shared half 
duplex 10Mbps Ethernet link rather than the modern standards of full
duplex 100Mbps and 1Gbps. It might also apply if you were using
quite ancient and underpowered Ethernet switching equipment that was
being overwhelmed by a flood of frames, or did not use the modern
"Cut Through" switching technique that is enabled in virtually all
switching equipment made this century. It may also apply if you
were using links with a capacity significantly greater than 1Gbps,
however there's no way a Seagate Central would ever approach that
speed of data throughput.

See the following links for more discussion of Jumbo frames.

Jumbo Frames: Do You Really Need Them?
https://www.mirazon.com/jumbo-frames-do-you-really-need-them/

The Great Jumbo Frames Debate
http://longwhiteclouds.com/2013/09/10/the-great-jumbo-frames-debate/

####  USB On-The-Go (OTG)
USB On-The-Go (OTG) is a mechanism whereby a USB "Device" like a 
smart phone can act as a USB "Host" so that it can connect to other
USB devices like a storage drive or a keyboard. Presumably in the 
context of the Seagate Central, it would let the NAS act as a 
"Device" that could connect to a "Host" like a Phone or PC. This way
the Phone or PC could directly access the data stored on the NAS 
via the USB connection. 

I'm confident that the Seagate Central has never supported OTG however
however it is functionality that was included in the original Seagate 
Central Kernel.

OTG isn't mentioned in the Seagate Central documentation and I cannot
find any examples online of this kind of functionality being used with
a Seagate Central. In addition there doesn't seem to be any method to
configure or customize this feature, such as which user's Data folder
would be made available over the OTG connection and so forth.

I'm not brave enough to test this functionality by plugging my Seagate
Central into my PC or Phone via a reversed polarity male to male USB
cable because I'm scared that I'll end up frying something!!

If anyone has actually used this kind of functionality on a Seagate 
Central then I'd be interested to hear about it and details of *exactly*
how you got it to work.

#### PCI, SDHCI, XHCI, and other hardware components not used by the Seagate Central
The Cavium CNS3420 CPU, which the Seagate Central is based on, supports
a range of different hardware bus types and devices however the Seagate
Central does not make use of all of these.

I've focused my efforts on only porting over support for the hardware 
components that are present on the Seagate Central Single Hard Drive NAS. 

## Acknowledgements
This project is based on the Seagate Central Firmware GPL
source code available at

https://www.seagate.com/gb/en/support/downloads/item/central-gpl-fw-master-dl/

It is also based on the excellent work by KL-Yang in the 
Seagate_central_linux (Single Disk Version) project at

https://github.com/KL-Yang/seagate_central_linux

The 64K memory page size work is largely based on the work by
Gregory Clement as documented at

https://lwn.net/ml/linux-arm-kernel/20200611134914.765827-1-gregory.clement@bootlin.com/

Thank you to all the greater minds than mine that have paved 
the way for this project.

## Postscript
I acknowledge that I haven't done things in the most elegant manner with 
these patches and code. For example there are a lot of "#ifdef" statements
introduced into some core kernel source files that get things working, but
aren't necessarily very graceful. For these reasons I don't plan to make 
any effort to submit these patches to the mainline Linux kernel. I also
don't make any commitment to maintain this project going forward as future 
linux versions beyond v5.x are released.

I confess that I haven't always fully comprehended every piece of code 
I've ported over in order to get this project working. It may be that 
there are flaws in the original code that I've blindly pulled over. It
may be that there are flaws that I've created myself through lack of 
understanding. As I mentioned, my goal was simply to "get things working".

Hopefully these instructions can serve as a template for upgrading the 
linux kernel on other Linux based embedded NAS equipment. In particular 
the 64K page size additions will hopefully prove useful for other NAS
products that chose to use 64K pages in their file systems.

Finally I learned a great deal about Linux, arm32 and embedded systems 
while writing this guide. Please read these instructions with the 
understanding that I am still in the process of learning. I trust that 
this project will help others to learn as well.

