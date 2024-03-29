# README_MANUAL_KERNEL_INSTALLATION.md

This is a guide that describes how to manually replace the original
Linux v2.6.25 kernel on a Seagate Central NAS with a previously
cross compiled modern Linux kernel.

Refer to the instructions in **README_CROSS_COMPILE_KERNEL.md**
to cross compile a kernel for use in this procedure or refer to the
**README.md** file for the location of a precompiled kernel binary.

The target platform tested was a Seagate Central Single Drive NAS 
running firmware version 2015.0916.0008-F however I believe these 
instructions should work for other Seagate Central firmware versions
as long as care is taken to account for any minor differences.

These instructions should not be followed "blindly". If you have 
already made other custom changes to your Seagate Central software via
the command line, such as installing other cross compiled software,
then make sure that none of the steps below interfere with those 
changes.

## TLDNR
* Obtain a uImage kernel image from the releases section in the project (or build one yourself).
* Upload the uImage to the unit being upgraded.
* Perform optional customizations and optimizations.
* Identify then mount the boot partition (either /dev/sda1 or /dev/sda2).
* Remove the old kernel and install the new one.
* Reboot the unit by shutting down and power cycling.

## Prerequisites
### ssh access to the Seagate Central.
You'll need ssh access to issue commands on the Seagate Central command 
line. 

If you are especially adept with a soldering iron and have the right 
equipment then you could get serial console access but this is quite
difficult and is **not required**. There are some very brief details 
of how to attach a serial console at

http://seagate-central.blogspot.com/2014/01/blog-post.html

Archive : https://archive.ph/ONi4l

That being said, if you are interested in doing any software development
or experimentation with the kernel then having console access is invaluable 
for the purposes of troubleshooting problems during kernel boot. I would 
suggest keeping all the cables coming from the circuit board as short
as possible and to expect a number of distortions in the serial interface
output.

### su/root access on the Seagate Central.
Make sure that you can establish an ssh session to the Seagate Central
and that you can successfully issue the **su** command to gain root
privileges. Note that some later versions of Seagate Central firmware
deliberately disable su access by default.

If you do not have su access on your Seagate Central then there are
instructions at the following URL that can help you to attain it.

https://github.com/bertofurth/Seagate-Central-Tips/blob/main/Reset-SU-Root-Password.md

### Know how to copy files between your host and the Seagate Central. 
Not only should you know how to transfer files to and from your Seagate
Central NAS and the build host, ideally you'll know how to transfer files 
**even if the samba service is not working**. I would suggest that if
samba is not working to use FTP or SCP which should both still work.

### Samba version on the Seagate Central
Although this is not strictly a pre-requisite of this kernel installation
procedure, the original samba file sharing service on the Seagate Central 
will have slightly imparied performance after the new kernel is installed.
We suggest installing an up to date and modern version of samba. 

See the Seagate-Central-Samba project at the following link for more
details and instructions on how to upgrade the samba service on the
Seagate Central.

https://github.com/bertofurth/Seagate-Central-Samba

## Procedure
### Workspace preparation
If not already done, download the files in this project to a new
directory on your build machine. 

For example, the following **git** command will download the files
in this project to a new subdirectory called 
Seagate-Central-Modern-Slot-In-Kernel

    git clone https://github.com/bertofurth/Seagate-Central-Modern-Slot-In-Kernel
    
Alternately, the following **wget** and **unzip** commands will 
download the files in this project to a new subdirectory called
Seagate-Central-Modern-Slot-In-Kernel-main

    wget https://github.com/bertofurth/Seagate-Central-Modern-Slot-In-Kernel/archive/refs/heads/main.zip
    unzip main.zip

Change into this new subdirectory. This will be referred to as 
the base working directory going forward.

     cd Seagate-Central-Modern-Slot-In-Kernel

### Transfer the cross compiled kernel to the Seagate Central
You should have a self generated or downloaded kernel "uImage" file
which you'd like to install on the Seagate Central. 

If you have cross compiled the kernel as per  
**INSTRUCTIONS_CROSS_COMPILE_KERNEL.md** in this project then the
generated "uImage" kernel file will be in the following
location relative to the base working directory.

    obj/arch/arm/boot/uImage

If you have downloaded a pre-compiled kernel from the releases
section of this project with a name like "uImage.v6.1.X-sc"
then it is suggested that the file be renamed to "uImage" at this
point. For example

     mv uImage.v6.1.X-sc uImage

Transfer this image to the Seagate Central. In this example we use 
the scp command however, any other means can be used. When using scp 
you will need to substitute your own username and NAS IP address.

    scp obj/arch/arm/boot/uImage admin@192.0.2.99:/Data/admin/

After executing the scp command you'll be prompted for the user's
password.

### Optional (Not usually needed) - Transfer kernel modules to the Seagate Central
The instructions in this project have been designed so that
kernel modules are not necessary for the new kernel image
to function properly. Steps in this procedure dealing with
modules are optional and should only be executed if you have
manually built modules and are confident in your understanding of
how to manage and use them.

If kernel modules have been built then I would suggest archiving 
the directory they are stored in on the build host and then 
transferring the archive to the Seagate Central as per the following
example

     tar -caf cross-mod.tar.gz cross-mod/
     scp cross-mod.tar.gz admin@192.0.2.99:/Data/admin/

### Transfer the config and script patches to the Seagate Central
A number of configuration and script files need to be patched on
the Seagate Central in order to cater for the new kernel's requirements.

Relevant files are included in the base directory of this project 
and have a suffix of ".SC.patch" and ".sh". These files need to be 
transferred to the Seagate Central. This can be done via scp as per the 
following example.

     scp *.SC.patch *.sh admin@192.0.2.99:/Data/admin/
     
### Login to the Seagate Central 
Establish an ssh session to the Seagate Central. All the commands
after this point in the procedure are executed on the Seagate
Central. Change to the directory where the patch and script files
were copied to the unit in the previous step. For example

    admin@NAS-X:~$ cd /Data/admin
    admin@NAS-X:/Data/admin$

### Login as root or prepend sudo to further commands
The commands after this point in the procedure must be executed with
root privileges on the Seagate Central. This can be done by either 
prepending **sudo** to each command or by issuing the **su** command
and becoming the root user. For example

    admin@NAS-X:/Data/admin$ su
    Password: <Enter-Password>
    root@NAS-X:/Data/admin#
     
### Optional (Recommended) - Disable obsolete services
The Seagate Central comes with a number of services that have
become obsolete and defunct. These include the "Seagate Media Service"
and "Tappin Remote Access Service". These services consume a considerable
amount of memory and CPU resources but do not provide any useful
functionality.

We strongly suggest disabling these services by issuing the following
commands as the root user on the Seagate Central.

    update-rc.d -f media_server_daemon remove
    update-rc.d -f media_server_ui_daemon remove
    update-rc.d -f media_server_allow_scan remove
    update-rc.d -f media_server_default_start remove
    update-rc.d -f tappinAgent remove

See the following URL for more details

https://github.com/bertofurth/Seagate-Central-Tips/blob/main/Disable_obsolete_services.md

### Optional (Recommended) - NTFS/exFAT USB insertion 
In order to take advantage of the new exFAT and NTFS file system
support in the Linux kernel the following patch must be applied
to one of the scripts that controls automatic mounting of newly
inserted USB devices. FAT32 USB drives will still work regardless
of whether this patch is applied.

The original script should first be backed up. Next, it should be patched
using the "usbshare.py.SC.patch" patch file that was transferred to the
Seagate Central in a previous step.

     cp /usr/lib/python2.6/site-packages/shares/usbshare.py /usr/lib/python2.6/site-packages/shares/usbshare.py.old
     patch -i usbshare.py.SC.patch /usr/lib/python2.6/site-packages/shares/usbshare.py

### Optional (Recommended) - Network IRQ CPU Affinity
Testing showed that by forcing the networking interrupts to use
the second CPU in the unit (CPU 1), there was a small but still
statistically significant speed improvement for smb file transfers
from a client to the the Seagate Central. (~51.5MB/s vs ~53.4MB/s)

For this reason we suggest installing the included netdev-cpu.sh
startup script on the upgraded Seagate Central which will set 
the networking functions to have an affinity for CPU 1 rather than
the default of both CPU 0 and CPU 1.

The "netdev-cpu.sh" script, which was copied to the Seagate Central in
a previous step, can be installed as a startup script using the
following commands.

    cp netdev-cpu.sh /etc/init.d/
    chmod 755 /etc/init.d/netdev-cpu.sh
    ln -s ../init.d/netdev-cpu.sh /etc/rcS.d/S42netdev-cpu.sh

### Optional - IPv6
The original Seagate v2.6.35 kernel did not support IPv6 at all 
so many components of the Seagate Central system are not, by
default, configured to properly operate in an IPv6 environment.

If you would like to take full advantage of IPv6 functionality 
in the new kernel then a number of service startup and
configuration files need to be modified. 

If you would prefer to keep using only IPv4 as per the original 
Seagate Central firmware, or if you have no need for or 
understanding of IPv6 functionality then it's not necessary to
complete the steps in this section.

If you're not sure about whether you want to implement IPv6 on
the Seagate Central, then you can skip this section for the moment 
and try it later after you've successfully upgraded the kernel.

#### Bounce IPv6 on eth0 at startup
The Seagate "networklan" daemon is in charge of monitoring the
Ethernet interface state and providing it's configuration. This
tool deliberately removes IPv6 address configuration on the
ethernet interface as the original Seagate Central firmware does
not support IPv6.

To overcome this, it is necessary to turn IPv6 off then back on
for the ethernet interface after the networklan tool is
invoked.

The "ipv6_bounce.sh" script, which was copied to the Seagate Central in
a previous step, needs to be installed as a startup script using the
following commands.

    cp ipv6_bounce.sh /etc/init.d/
    chmod 755 /etc/init.d/ipv6_bounce.sh
    ln -s ../init.d/ipv6_bounce.sh /etc/rcS.d/S90ipv6_bounce.sh

These commands simply make sure the script is installed in the correct
directory, is marked as executable and is listed as executing with
priority "90". 
    
Note that this script must be numerically ordered to execute after
the /etc/rcS.d/S41blackarmor-network startup script which starts the
networklan daemon.

#### Patch service configuration files for IPv6
In order for some services on the Seagate Central to use IPv6
their configuration files need to be slightly modified.

Note that only those services you wish to access via both IPv6
and IPv4 need to have their configuration patched. Also note 
that if you revert the Seagate Central back to the original
v2.6.35 kernel then you'll need to also revert the configuration
files that are modified in this section back to their original 
states otherwise they won't work at all.

Backup and patch the required service configuration files with
the patches copied to the Seagate Central in a previous step as 
per the following examples.

     # AFP - Apple Filing Protocol
     cp /etc/netatalk/afpd.conf /etc/netatalk/afpd.conf.old
     patch -i afpd.conf.SC.patch /etc/netatalk/afpd.conf
     
     # AVAHI - "Zero Conf" mDNS service
     cp /etc/avahi/avahi-daemon.conf /etc/avahi/avahi-daemon.conf.old
     patch -i avahi-daemon.conf.SC.patch /etc/avahi/avahi-daemon.conf
     
     # Lighthttpd - Web management interface
     cp /etc/lighttpd.conf /etc/lighttpd.conf.old
     patch -i lighthttpd.conf.SC.patch /etc/lighttpd.conf
     
     # FTP/SFTP server - Legacy file sharing protocols
     cp /etc/vsftpd.conf /etc/vsftpd.conf.old
     patch -i vsftpd.conf.SC.patch /etc/vsftpd.conf
     
### Installing the new kernel
Here is where we actually put the new kernel into place. It's important
to execute the steps in this section correctly. **Please try to read the
explanation of what each step is trying to achieve and understand what 
you are doing before executing any of the following commands.**

#### Find out which copy of firmware is active
The Seagate Central keeps two copies of firmware available on the
hard drive in case one copy becomes corrupted and cannot boot. In
this step we need to discover which of the two copies is currently
active on the Seagate Central so that we can modify the active one.

The name of the currently active kernel is stored in the flash memory
of the Seagate Central using a variable called **current_kernel** . When
the bootloader program, u-boot, becomes active just after the Seagate
Central is switched on, it checks the value of this variable in order
to see which one of the two copies of firmware should be booted.

We can check the value of this variable with the following command
run on the Seagate Central command line

    fw_printenv | grep current_kernel

If the first/primary copy of firmware is active then the output of the command will say

    current_kernel=kernel1
     
If the second copy of firmware is active then the output of the command will say

    current_kernel=kernel2

**Take note of which copy of firmware is currently active.**

Interesting note: It is possible to manually change the value of this variable
in order to force the Seagate Central to switch to using the other copy of
firmware on reboot by using the "fw_setenv current_kernel kernel1" or 
"fw_setenv current_kernel kernel2" command. Keep this in mind if anything goes
particularly wrong!

#### Mount the kernel boot partition
The kernel image is kept on a disk partition which is not, by default, mounted
by the Seagate Central during normal operation. We will call this partition
the **kernel boot partition**.

If the **first** copy of firmware is active (current_kernel=kernel1) then the kernel boot 
partition is located on "/dev/sda1" . Mount the kernel boot partition with the
command

    mount /dev/sda1 /boot
     
If the **second** copy of firmware is active (current_kernel=kernel2) then the kernel boot 
partition is located on "/dev/sda2" . Mount the kernel boot partition with the
command

    mount /dev/sda2 /boot
     
#### Make a backup copy of the original kernel
Create a backup copy of the original kernel as follows.

    cp /boot/uImage /boot/uImage.old
     
#### Copy the new kernel into place
Remove the old kernel uImage then copy the new kernel uImage into place so
that on next boot it will be loaded by the system. Note that the active kernel
image in the /boot/ directory must be called "uImage" otherwise it will not
be loaded by the system on boot.

    rm -f /boot/uImage
    cp uImage /boot/uImage

Confirm that the new uImage file is in place

    ls -l /boot
     
The output of the above command should show the new kernel and the backed up
original kernel as per the following example. Note the new kernel will generally 
be about 4 - 5 MB in size, whereas the original kernel is much smaller at
about 2.9MB.

    total 6995
    drwx------ 2 root root   12288 Oct  6 21:27 lost+found
    -rw-r--r-- 1 root root 2989612 Oct  6 21:29 uImage
    -rw-r--r-- 1 root root 4110824 Oct  7 08:04 uImage
    
#### Optional (Not usually needed) - Copy kernel modules into place
This step should only be performed if you have made your own custom changes
to the procedure in order to build kernel modules. By default, no kernel
modules need to be built or installed so this step can be skipped.

If you have an archive of kernel modules associated with the newly installed
uImage then copy it to the Seagate Central, extract it and check to make sure
that the extracted directory contents are as expected. 

     tar -xf cross-mod.tar.gz

Under the lib/modules subdirectory, there should be another directory 
containing the modules for the kernel version being installed. Under
that subdirectory there should be more module configuration files
and a "kernel/" subdirectory containing the module tree. For example

    #ls -p cross-mod/lib/modules/
    6.1.69-sc/
    #ls -p cross-mod/lib/modules/6.1.26-sc
    build
    kernel/
    modules.alias
    modules.alias.bin
    modules.builtin
    modules.builtin.alias.bin
    modules.builtin.bin
    modules.builtin.modinfo
    modules.dep
    modules.dep.bin
    modules.devname
    modules.order
    modules.softdep
    modules.symbols
    modules.symbols.bin
    source

After checking that the modules have been extracted as expected, install
the new modules by copying the module tree into place as per the following
example.

Note that this is a **very** dangerous part of the process so if you don't
understand what you are doing here then do not proceed with the following
command.

     cp -r cross-mod/lib/modules/* /lib/modules/

There should be a new 6.1.x-sc modules subdirectory on the unit alongside the 
modules subdirectory for the original v2.6.35 kernel. The output of the 
following command

     ls -l /lib/modules

should show an output similar to the following.

     drwxrwxr-x 4 root root 4096 Sep 17  2015 2.6.35.13-cavm1.whitney-econa.whitney-econa
     drwxr-xr-x 3 root root 4096 Sep 28 14:47 6.1.69-sc
     
Finally remove the original "modules.dep" file in the new module 
subdirectory. Removing this file will cause the unit to perform a
"depmod" for the newly installed modules on next boot which will
properly index them.

     rm /lib/modules/6.1.69-sc/modules.dep
     
### Reboot and confirm the upgrade    
Finally, we reboot the unit and confirm that the new kernel is
operational.

At this point we suggest shutting down the unit and power cycling
as opposed to a "soft" reboot.

Shutting down the unit can be performed via the Seagate Central command
line with the "shutdown -h now" command.

     shutdown -h now
     
Naturally, at the point when the unit is shutdown any ssh sessions to
the Seagate Central will be disconnected.

Wait for a minute for the unit to shutdown properly, then power down the
unit by disconnecting the power supply from the unit or from the mains.
Reconnect the power after a few seconds.

The unit should take about 3 or 4 minutes to reboot. The indicator light
on the top of the system should go from amber, to flashing green and then
eventually show a solid green. After the LED has been solid green for about
1 minute the unit will hopefully be operational and running the new Linux
kernel.

Try to re-establish an ssh connection to the newly upgraded Seagate Central.
If you cannot re-establish a connection or if the green LED does not go solid
then manually power cycle the unit again by disconnecting the power for a
few seconds and trying again. See the Troubleshooting section below if you
still cannot reestablish an ssh connection.

After re-connecting to the unit via ssh, issue the following command to confirm 
that the unit has loaded the new kernel.

     uname -a
         
The output should indicate that the version of the running kernel is now
6.1.x-sc and that SMP functionality is enabled, as per the following sample
output.

     Linux NAS-X 6.1.69-sc #1 SMP Sun Dec 31 09:25:15 AEDT 2023 armv6l GNU/Linux
     
Further confirm that the services you wish to make use of on the Seagate Central
are functional, including

* The Web Management interface
* Samba / Windows file sharing 
* FTP/SFTP server
* DLNA media streaming service

### Optional - Revert back to the original kernel
If the new kernel version is not performing as desired then there is always 
the option of reinstating the original version.

If the procedure above has been followed then the sequence of
commands below issued with root privileges on the Seagate Central 
will restore the original kernel version.

First, mount the correct kernel boot partition as per the instructions 
in the "Installing the new kernel" section.

     mount /dev/sda1 /boot
     
OR

     mount /dev/sda2 /boot
     
Next, restore the original kernel image.     
     
     cp /boot/uImage.old /boot/uImage

Restore the original version of the usb drive automount script

     cp /usr/lib/python2.6/site-packages/shares/usbshare.py.old /usr/lib/python2.6/site-packages/shares/usbshare.py

If service configurations have been modified to enable IPv6 then
these will need to be reverted back to their original versions.

     cp /etc/netatalk/afpd.conf.old /etc/netatalk/afpd.conf
     cp /etc/avahi/avahi-daemon.conf.old /etc/avahi/avahi-daemon.conf
     cp /etc/lighttpd.conf.old /etc/lighttpd.conf
     cp /etc/vsftpd.conf.old /etc/vsftpd.conf
          
Reboot the unit with the reboot command

     reboot

Once the unit has rebooted, log in via ssh and confirm that the original 
kernel is back in place by issuing the "uname -a" command

     uname -a
     
The command output should indicate that kernel version has reverted back to
2.6.35 as per the following sample output.

    Linux NAS-X 2.6.35.13-cavm1.whitney-econa.whitney-econa #1 Wed Sep 16 15:47:59 PDT 2015 armv6l GNU/Linux

## Troubleshooting
The most common issue that may occur after a failed kernel upgrade
is that the unit is no longer has network connectivity. If this happens then
the first thing you should try is manually power cycling the unit. That is,
disconnect the power supply and then reconnect it.

There have been reports that sometimes after an upgrade the unit needs 
to have the power supply disconnected then reconnected for the Ethernet
to work.

If power cycling the unit once or twice does not resolve the problem then 
the next suggested course of action is to force the unit to revert to it's
backup firmware as per the following procedure

1) Power down then power up the Seagate Central by disconnecting and reconnecting the power.
2) Wait about 30 seconds to a minute for the LED status light on top of the unit to turn from solid amber to flashing green. 
3) As soon as the LED starts flashing green execute the first 2 steps three more times in a row.
4) On the 4th boot up let the unit fully boot. It should now load the backup / alternate version of firmware

Make sure that step 2 is followed correctly. That is, power off the 
unit as soon as the LED status light starts flashing green. Don't
let it proceed to the solid green state.

After the unit boots up with the alternate version of firmware, take
steps to mount the "failing" kernel boot partition and revert it back
to the original kernel uImage.

If the unit is accessible via ssh then the best places to search for
troubleshooting data includes the system bootup log as displayed by
the **dmesg** command and the system log stored at /var/log/syslog

If some services are functional but others are not then check the 
log files pertinent to the failing services as well as relevant messages
in the the above mentioned log files.

* samba - /var/log/log.smbd  and /var/log/log.nmbd
* FTP/SFTP - /var/log/vsftpd.log
* Web Management Interface - /var/log/user.log
