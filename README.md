# Seagate Central Slot In v5.x Kernel
# Summary

A modern slot in Linux Kernel for the Seagate Central Single
Drive NAS running stock Seagate Central firmware.

This project accompanies the **Seagate-Central-Samba** project
at the following link

https://github.com/bertofurth/Seagate-Central-Samba

Installing the samba software at the above project is
a strongly recommended pre-requisite of this project. If you want
Windows style samba network file sharing to work then you must 
upgrade the samba service on the Seagate Central before, or at
the same time as upgrading the Linux kernel.

Additional servers and utilities for the Seagate Central may
be found in the **Seagate-Central-Utils** project at the
following link

https://github.com/bertofurth/Seagate-Central-Utils

A pre-compiled Linux v5.x.x kernel based on the instructions
in this guide is available in the "Releases" section of this 
project at

https://github.com/bertofurth/Seagate-Central-Slot-In-v5.x-Kernel/releases

There are three sets of instructions included in this project.

### README_CROSS_COMPILE_KERNEL.md
Cross compile the new v5.x Linux kernel for Seagate Central from 
scratch.

### README_USB_DEVICE_MODULES.md
Modify the kernel configuration to add support for new USB devices
such as a video camera.

### README_MANUAL_KERNEL_INSTALLATION.md
Manually install the new Linux kernel onto the Seagate Central.

Also note that the **Seagate-Central-Samba** project has a component 
that allows the easy upgrade of the system's samba software via the
Web Interface controlled firmware upgrade process. This can also be
used to upgrade the Linux kernel on a Seagate Central. See

https://github.com/bertofurth/Seagate-Central-Samba/blob/main/README_FIRMWARE_UPGRADE_METHOD.md

## Details
This project's main goals are

* Develop a new and modern version of the Linux kernel for Seagate Central.
* Make the installation as seamless as possible.
* Allow users to keep using the existing 64K page formatted Data volume.
* Allow users to continue using the Seagate supplied services.
* Allow users to add new services to the Seagate Central.

The hope is that by providing a modern kernel to replace the old 
v2.6 native kernel, users will be able to add new modern software 
services to their Seagate Central without having to go through the
dangerous and tedious process of installing an entirely new operating 
system.

The main obstacle this project had to overcome was incorporating 
support for 64K memory page sizes. This was required because the native
disk format for the user Data partition on a Seagate Central uses 64K
pages. These are only supported natively by Linux when the memory pages
are 64K or larger. This goal was largely achieved by drawing on the work
of Gregory Clement (see Acknowledgements).

The other notable features that this project focused on include
* USB support - Allows connection of external drives and other devices.
* Symmetric Multiprocessing (SMP) - More efficient use of CPU resources.
* IPv6 - The new standard for internet networking.
* exFAT/NTFS3 - New Linux support for exFAT and NTFS formatted USB drives.
* Real time clock (RTC)
* Access to u-boot environment in flash
* Ethernet mac-address stored in flash
* Control of status indicator LED
* Access to the "Factory Default" button
* A fake "sgnotify" device 

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

The Seagate Central version of the boot loader (u-boot) has a feature 
where it automatically reverts to the previous version of firmware if
it finds it is unable to successfully bootup the system after 4 consecutive
attempts. This normally overcomes any kind of cataclysmic software induced
failure.

In the absolute worst case where a Seagate Central were rendered totally
inoperable there is always the option of physically opening the Seagate
Central, removing the hard drive and mounting it on a different machine
in order to resurrect it, and if necessary, retrieve any user data. See

https://github.com/bertofurth/Seagate-Central-Tips/blob/main/Unbrick-Replace-Reset-Hard-Drive.md

Naturally the most prudent thing to do is to make a backup of the user
Data on the Seagate Central before attempting this procedure, but I'll
emphasize again that I have never lost any data on a Seagate Central during
the course of developing or testing this procedure.

## Caveats
There are some aspects of the Seagate Central that make it difficult
to create a truly "plug-in" kernel replacement. They are listed in
detail here in approximate order of importance. 

#### Potential Ethernet/Networking issues on bootup
Summary : On rare occasions the unit needs to be power cycled and not
just soft rebooted to avoid Ethernet networking issues.

Some users have reported that sometimes after the unit is rebooted it
may need to be power cycled in order for the Ethernet interface to
come up. 

In addition, when the Ethernet cable is physically disconnected from
the unit the system does not properly recognize this event. Instead, the
system will continue to act as if the Ethernet interface is still "up".
After the unit is physically reconnected to the Ethernet LAN it
will simply function as if nothing has happened. Normally this doesn't
cause any problems. One very rare case where this might be an issue
would be where a Seagate Central is moved from one physical subnet to
another while still being powered on but this would be an unlikely scenario.

Another scenario where a problem may sometimes occur is if the IP address
configuration of the unit is modified via the Web Management interface.
After reconfiguration, IPv6 (which is a new capability of the new kernel)
may not function properly.

It may be that the best course of action is to power cycle the unit
after an IP address configuration change, if the unit is deliberately
physically reconnected to a different LAN or if the underlying 
configuration of the network the unit is connected to is changed.

TODO : Modify the startup scripts so that if the unit detects that the
ethernet hasn't aquired an IP address in a reasonable amount of time,
we "bounce" the ethernet interface and try again.

### Networking Performance
Summary : The raw receive networking performance in the new kernel is
less than the original kernel, however in practical terms the new SMP 
capability makes up for it.

The original Seagate Central Linux Kernel (2.6.35.13-cavm1.whitney-econa)
included a proprietary module called sopp_cns3xxx_nas which dedicated
one of the two CPU cores on the unit entirely to the networking function.
This meant that the original Seagate Central kernel had very fast networking
performance, particularly when the Seagate Central was receiving packets. 
(iperf3 rx tcp : ~906Mbps)

The drawback of the sopp_cns3xxx_nas approach was that other Linux processes
could only make use of the one remaining CPU core, so when multiple processes
were active at once, their performance may have suffered.

The new v5.x kernel is not able to make use of the Seagate proprietary 
networking module and as such, raw networking performance for received
packets is not quite as fast as with the original kernel (iperf3 rx tcp 
: ~663Mbps)

The new and old kernel's raw transmit packet performance levels are
essentially identical (iperf3 tx tcp : ~401Mbps)

It is worth noting that when the new v5.x kernel is combined with the new
samba 4.x server as seen in the **Seagate-Central-Samba** project,
then both the upload and download performance of the smb file service is
improved.

See the following URL for some statistics regarding networking
and file transfer performance.

https://github.com/bertofurth/Seagate-Central-Tips/blob/main/Network_Performance.md

### Minor caveats
The issues below are unlikely to impact on the normal operation of the
Seagate Central, however for completeness sake they are documented here.

### NTFS formatted external drives
Summary : We recommend using FAT32 or exFAT instead of NTFS for external
USB drives.

NTFS is a commonly used filesystem for large hard drives formatted using 
modern Windows operating systems.

The original Seagate Central firmware uses a proprietary driver module, 
called ufsd, to mount and interact with USB attached NTFS drives. 

Unfortunately the v5.x slot in kernel is not able to make use of this
proprietary driver as it was designed for exclusive use with only the original
custom Seagate Central based Linux v2.6.35 kernel.

Since v5.15.x, Linux includes native support for a new read and write
capable NTFS driver called ntfs3 by Paragon Software. My own brief tests 
indicate that this driver works well however this driver is quite new and may
well still have some issues. See the following link for more details

https://www.paragon-software.com/home/ntfs3-driver-faq/

My suggestion is to format externally attached drives using FAT32, or very 
large drives using exFAT, rather than using NTFS. This ensures that your drive 
will be using reliable and well tested kernel drivers. In addition it means that 
your drive will be compatible with a larger range of other devices.

Please refer to the following links that go into much more detail.

* How-To Geek : What File System Should I Use For My USB Drive?
https://www.howtogeek.com/73178/what-file-system-should-i-use-for-my-usb-drive/

* Digital Citizen : What is FAT32 and why is it useful?
https://www.digitalcitizen.life/what-is-fat32-why-useful/

* Format a drive with FAT32 greater than 32G (up to 8TB)
https://www.howtogeek.com/316977/how-to-format-usb-drives-larger-than-32gb-with-fat32-on-windows/

#### No red blinking LED status light for lost Ethernet connectivity
Summary : With the new kernel, the status LED on top of the unit won't
blink red if the Ethernet loses connectivity.

When using native Seagate supplied stock firmware, if a Seagate Central
detects that the Ethernet connection is down then the status light on
the top of the unit will change from solid green to blinking red. This
will not occur when using the new kernel. This relates to the issue 
listed above with Ethernet disconnect handling.

Note that the other LED status states work properly. That is, solid
amber after power on, flashing green during the boot process, solid
green once the main bootup process is finished and flashing red
for a few seconds after the unit is commanded to reboot. 

Also note that the status LEDs on the Ethernet port itself work fine
and will properly indicate the status of the Ethernet port.

See the TODO section below dealing with "ledmanager" for more details
and an experimental partial resolution of this issue.

#### Kernel DEBUG and other obscure options (Rare)
In some cases there may be problems when the kernel is compiled using a
non default configuration using some less frequently used options such
as older kernel drivers or those under the "Kernel Hacking" menu of
the Linux menuconfig dialog.

I don't believe this is due to anything specific to this project but is
just a function of the ARM32 architecture combined with various corner
cases receiving less testing focus from the general Linux community in
recent times.

Some examples I've encountered in my testing.

* CONFIG_KASAN will cause a hang on boot.

* CONFIG_FORTIFY_SOURCE may cause a hang on boot. (Fixed with patch 0007)

* CONFIG_GCC_PLUGINS and/or CONFIG_STACKPROTECTOR_PER_TASK causes panics.

* CONFIG_PREEMPT_NONE will stop the Ethernet network from working. 

* CONFIG_AFS_FS, the rarely used Andrew File System, will not work with 64K memory pages.

* SMP may not work in 4K page size mode with CONFIG_DEBUG_PAGEALLOC_ENABLE_DEFAULT enabled.

* CONFIG_BPF_STREAM_PARSER causes tracebacks with 64K page size.

The rule of thumb I have had to follow when encountering "weird" 
issues is to try compiling the kernel with the least number of debugs
and "rare" features turned on as possible.

#### Unusual tracebacks and warnings on bootup 
A careful examination of the bootup logs (dmesg) may reveal some warning messages
and tracebacks. 

These messages do not seem to impact on the main functionality of the unit.

Some examples of these traceback warning messages include include

    WARNING: CPU: X PID: XX at mm/vmalloc.c:XXX vunmap_range_noflush+0xXX/0xXX
    . . .
    (warn_slowpath_fmt) from [<XXXXXXXX>] (vunmap_range_noflush+0xXX/0xXXX)
    . . .
    (bpf_prog_free_deferred) from [<XXXXXXXX>] (process_one_work+0xXXX/0xXXX)

At this stage I'm not certain as to the cause of the messages. They only
appear when the 64K page version of the kernel is in use and the BPF
subsystem is invoked. The Seagate Central does not appear to make extensive 
use of the BPF subsystem in normal operation so hopefully this won't be an
issue.

My research indicates that the "warn_slowpath_fmt" messages may be indicative
of one process blocking another from freeing memory straight away but I'm not
at all sure.

Note that for the purposes of not polluting the logs, the first of these messages
is printed but subsequent messages of the same type are suppressed.

### TODO : (But probably not)
Here are some components that I *could* have put some further effort into,
and may be inclined to at some later date.

#### Replace the proprietary "networklan" daemon
The Seagate Central uses a proprietary and closed source tool called
"networklan" to control and monitor the state of the Ethernet network
interface.

This tool does not work well with the new kernel. There are a number
of alternative Ethernet management tools available, however it's unlikely
that they would be backwards compatible with the original kernel.

Ideally this "networklan" tool should be completely replaced, however
the tool seems to work well enough in most normal scenarios if one bears
in mind the minor issues listed in the caveats section above.

#### Replace the proprietary "ledmanager" daemon
The Seagate Central uses a proprietary tool called "ledmanager" to
manage the state of the status LED on the top of the unit.

This tool has some slight incompatibilities with the new kernel which
can sometimes cause cosmetic tracebacks to appear in the system logs.

There is an experimental script that is included in the base directory
of this project called **new-led-monitor.sh** that could serve
as the basis for a replacement led status monitor. Note that this
script has not been thoroughly tested but might serve as the basis
for future work.

#### Power Management 
In the new kernel there's no ability to suspend or to power down one of
the CPUs during low CPU usage. Note that the old kernel had no such 
functionality either.

It's possible to implement CPU power management but in my experience a
file server needs to be ready to go 100% of the time so it's probably
not a feature that would get much use.

#### Real Time Clock
While the real time clock implementation works very well with the 
Seagate Central firmware and the included version of the "hwclock"
utility, it has a slight incompatibility with the generic "hwclock"
tool which I have not been able to resolve. 

The workaround implemented was to compile the kernel with the
CONFIG_RTC_HCTOSYS and CONFIG_RTC_SYSTOHC options which let the kernel
itself automatically read and write the time to the real time clock
rather than having to rely on the "hwclock" userspace utility.

#### 64K to 4K partition block size conversion procedure 
This project supports the 64K block sizes on the Seagate Central data
partition by introducing support for a non-standard memory page size 
of 64K.

While using 64K pages, as opposed to standard 4K pages, has a significant
performance benefit when it comes to disk operations, it means that the
system is also significantly less memory efficient. If a user wanted to
have a Seagate Central unit running multiple CPU and memory intensive 
services as opposed to acting as a simple file server, then using a 4K
kernel page size may potentially be a better choice.

One possibility might be to create a guide for converting the user data
volume in the Seagate Central from 64K block size to a standard 4K block
size. That way a standard memory page size of 4k could be used in the
kernel which would be slightly more efficient in terms of memory and
disk space usage.

It would also mean that the changes in this kernel would be much easier
to port into future releases of Linux because one of the most complicated
parts of implementing this project was getting 64K page support to work.

#### Jumbo Ethernet Frames 
Jumbo Ethernet frames allow devices to transmit frames that contain 
more than the standard data payload of 1500 bytes. This is in order 
to reduce the packet header processing overhead associated with 
each packet.

I have never come across any normal home or small enterprise network
that uses Jumbo Ethernet frames. One important reason for this is 
because Jumbo Ethernet frames are not supported over Wi-Fi. Jumbo 
frames only work over hard Ethernet connections where *all* the
infrastructure between the server and client (switches and routers)
support it. In most home or small enterprise networks, like the ones
where a Seagate Central would typically be deployed, it's not certain
that all the networking infrastructure would have this capability.

Jumbo frames only have a noticeable impact in contexts where bulk
data transfers are *constantly* flowing over a hard ethernet network
and where microseconds count. I see some internet documentation 
claiming that you can get up to 30% performance improvement with 
Jumbo Frames enabled however my opinion is that a benefit would only
be noticed in a few rare corner cases.

One example might be if a network were running using a shared half 
duplex 10Mbps Ethernet link rather than the modern standards of full
duplex 100Mbps and 1Gbps. It might also apply if you were using
quite ancient and underpowered Ethernet switching equipment that was
being overwhelmed by a flood of frames, or did not use the modern
"Cut Through" switching technique that is enabled in virtually all
switching equipment made this century. 

It may also be of benefit if you were fully utilizing 1Gbps or greater
links over sustained periods of time however there's no way a single
Seagate Central would ever approach that level of data throughput.
Maybe if you had five Seagate Centrals all transferring data between five
separate clients each at the same time over a congested 1Gbps link there
may be a benefit to be seen. I'd love to get some feedback from anyone
who has encountered this scenario.

See the following links for more discussion of Jumbo frames.

Jumbo Frames: Do You Really Need Them?
https://www.mirazon.com/jumbo-frames-do-you-really-need-them/

The Great Jumbo Frames Debate
http://longwhiteclouds.com/2013/09/10/the-great-jumbo-frames-debate/

#### USB On-The-Go (OTG)
USB On-The-Go (OTG) is a mechanism whereby a USB "Device" like a 
smart phone or a tablet can act as a USB "Host" so that it can
connect to other USB devices like a storage drive or a keyboard.

Presumably in the context of the Seagate Central, it would let the
NAS act as a "Device" that could connect to a "Host" like a PC.
This way the PC could directly access the data stored on the NAS 
via the USB connection. That is, the Seagate Central would act
like a USB hard drive.

I'm confident that the Seagate Central has never supported USB OTG
however it is a capability that was actually included in the original
Seagate Central Kernel.

USB OTG isn't mentioned in the Seagate Central documentation and I cannot
find any examples online of this kind of functionality being used with
a Seagate Central. In addition, there doesn't seem to be any method to
configure or customize this feature, such as which user's Data folder
would be made available over the USB OTG connection and so forth.

I'm not brave enough to experiment with this functionality by plugging 
my Seagate Central into my PC or Phone via a reversed polarity male to
male USB cable because I'm scared that I'll end up frying something!

If anyone has actually used this kind of functionality on a Seagate 
Central then I'd be interested to hear about it and details of *exactly*
how you got it to work.

#### PCI, SDHCI, XHCI, and other hardware components not used by the Seagate Central
The Cavium CNS3420 CPU, which the Seagate Central is based on, supports
a range of different hardware bus types and devices however the Seagate
Central only makes use of a subset of these.

I've focused my efforts on only porting over support for the hardware 
components that are used and present on the Seagate Central Single Hard Drive 
NAS. 

## Acknowledgements
I am very grateful to the following sources who I've based this project on.

This work is primarily based on the Seagate Central GPL source code which I
am grateful to the Seagate corporation for dutifully publishing.

https://www.seagate.com/gb/en/support/downloads/item/central-gpl-fw-master-dl/

It is also based on the excellent work by KL-Yang in the 
Seagate_central_linux (Single Disk Version) project which focuses on
compilation of a new kernel as part of installing a new operating
system on the Seagate Central.

https://github.com/KL-Yang/seagate_central_linux

The 64K memory page size implementation is largely based on the work
by Gregory Clement from Bootlin.

https://lwn.net/Articles/822868/

Thank you to all the greater minds than mine that have paved the way for
this project.

## Postscript
I acknowledge that I haven't done things in the most elegant manner with 
these patches and code. For example, there are a lot of "#ifdef" statements
introduced into some core kernel source files that get things working, but
aren't necessarily very graceful. 

I confess that I haven't always fully comprehended every piece of code 
I've ported over in order to get this project working. It may be that 
there are flaws in the original code that I've blindly pulled over. It
may be that there are flaws that I've created myself through lack of 
understanding. As I mentioned, my goal was simply to "get things working".

For these reasons I don't plan to make any effort to submit these patches
to the mainline Linux kernel. I'm also not in a position to be able to make
any commitment to maintain this project going forward as future Linux versions
beyond v5.x are released. That being said, please feel free to submit
"Issues" and suggestions for improvement. If you do submit an issue then
please be prepared for "stupid" questions from me!!

My main motivation for this project was that even though the Seagate
Central is an "ancient" platform, it is still perfectly capable of being 
useful in a modern home network. After making the upgrades in this guide 
my Seagate Central is happily working as a file server, as a storage point 
for half a dozen security cameras, and I have cross compiled and installed
lots of new software and services as documented by the 
**Seagate-Central-Utils** project including 

* Syncthing - Modern file synchronization server and client
* Motion - Security camera analysis software
* MiniDLNA - To replace the bloated and CPU guzzling Twonky DLNA server
* Up to date versions of Linux command line tools

Since the unit isn't particularly fast it's better to make use of these
services "one at a time" but thanks to the excessive swap space available
on the platform, and thanks to the new SMP support, it all seems to hang
together well.

Hopefully these instructions can serve as a template for upgrading the 
Linux kernel on other arm 32 based embedded NAS equipment. In particular 
the 64K page size additions will hopefully prove useful for other NAS
products that chose to use 64K pages in their file systems.

Finally, I learned a great deal about Linux, arm32 and embedded systems 
while writing this guide. Please read these instructions with the 
understanding that I am still in the process of learning. I trust that 
this project will help others to learn as well.

