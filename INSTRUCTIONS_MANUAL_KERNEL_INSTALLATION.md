# INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md
This is a guide that describes how to manually replace the original
Linux v2.6.25 kernel on a Seagate Central NAS with a previously
cross compiled modern, Linux v5.x kernel.

Refer to the instructions in **INSTRUCTIONS_CROSS_COMPILE_KERNEL.md**
to cross compile a kernel for use in this procedure or refer to the
**README.md** file for the location of a precompiled kernel binary.

Installation of the cross compiled kernel in conjunction with samba
by using the easier but less flexible firmware upgrade method
is covered by
**INSTRUCTIONS_FIRMWARE_UPGRADE_KERNEL.md**

The target platform tested was a Seagate Central Single Drive NAS 
running firmware version 2015.0916.0008-F however I believe these 
instructions should work for other Seagate Central firmware versions
as long as care is taken to account for any minor differences.

These instructions should not be followed "blindly". If you have 
already made other custom changes to your Seagate Central software via
the command line, such as installing other cross compled software,
then make sure that none of the steps below interfere with those 
changes.

## Prerequisites
### ssh access to the Seagate Central.
You'll need ssh access to issue commands on the Seagate Central command 
line. 

If you are especially adept with a soldering iron and have the right 
equipment then you could get serial console access but this is quite
difficult and is **not required**. There are some very brief details 
of the connections required at

http://seagate-central.blogspot.com/2014/01/blog-post.html

Archive : https://archive.ph/ONi4l

That being said, if you are interested in doing further development of
this kernel then having console access is invaluable for the purposes
of troubleshooting problems during kernel boot.

### su/root access on the Seagate Central.
Make sure that you can establish an ssh session to the Seagate Central
and that you can succesfully issue the **su** command to gain root
priviledges. Note that some later versions of Seagate Central firmware
deliberately disable su access by default.

The alternative procedure detailed in 
**INSTRUCTIONS_FIRMWARE_UPGRADE_KERNEL.md** does not require su access
and will in fact automatically re-enable su access as a result of the
procedure.

### Know how to copy files between your host and the Seagate Central. 
Not only should you know how to transfer files to and from your Seagate
Central NAS and the build host, ideally you'll know how to transfer files 
**even if the samba service is not working**. I would suggest that if
samba is not working to use FTP or SCP which should both still work.

### Samba version on the Seagate Central
Although this is not strictly a pre-requisite of this kernel installation
procedure it is worth re-emphazing here that the samba file sharing service
on the Seagate Central will not work after the new kernel is installed unless
arrangements have been made to upgrade it.

See the README.md file in this project and the Seagate-Central-Samba 
project at the following link for more details and instructions on how to
upgrade the samba service on the Seagate Central.

https://github.com/bertofurth/Seagate-Central-Samba

That being said, it's possible to proceed past this point without updating
the samba service if you just want to give the new kernel a quick test.
You will still be able to use the web management service and most of the
other services on the Seagate Central.

## Procedure
### Workspace preparation
If not already done, download the files in this project to a new
directory on your build machine. 

For example, the following **git** command will download the files
in this project to a new subdirectory called 
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

### Transfer the cross compiled kernel to the Seagate Central
You should have a self generated or downloaded kernel "uImage" file
which you'd like to install on the Seagate Central. 

If you have just completed cross compiling the kernel as per the 
INSTRUCTIONS_CROSS_COMPILE_KERNEL.md in this project then the
generated "uImage" kernel file will be in the following
location relative to the base working directory.

    obj/arch/arm/boot/uImage

Transfer this image to the Seagate Central. In this example we use 
the scp command with the "admin" user to copy the kernel to the 
user's home directory however any other means can be used. 

    scp obj/arch/arm/boot/uImage admin@192.168.1.99:

When using scp you will need to substitute your own username and 
NAS IP address. After executing the scp command you'll be prompted
for the user's password.

### OPTIONAL - Transfer kernel modules to the Seagate Central
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
     scp cross-mod admin@192.168.1.99:

### Transfer the config and script patches to the Seagate Central
A number of configuration and script files need to be patched on
the Seagate Central in order to cater for the new kernel's requirements.

Relevant patch files are included in the base directory of this project 
and have a suffix of ".SC.patch". These files need to be tranferred to
the Seagate Central. This can be done via scp as per the following example.

     scp *.SC.patch admin@192.168.1.99:
     
### Login to the Seagate Central 
Establish an ssh session to the Seagate Central. All the commands
after this point in the procedure are executed on the Seagate
Central.

### Login as root or prepend sudo to further commands
The commands after this point in the procedure must be executed with
root priviedges on the Seagate Central. This can be done by either 
prepending **sudo** to each command or by issuing the **su** command
and becoming the root user.

### Samba
If you wish to have a working Windows style samba file sharing service
operational after the kernel upgrade then now is the point at which 
the new samba software needs to be manually installed and activated.
See the instructions in the Seagate-Central-Samba project at the
following link for details.

https://github.com/bertofurth/Seagate-Central-Samba

### NTFS/exFAT USB insertion
In order to take advantage of the new exFAT and NTFS file system
support in the linux kernel the following patch must be applied
to one of the scripts that controls automatic mounting of newly
inserted USB devices.

The script can be backed up and patched with one of the patch files
transferred to the Seagate Central in a previous step as per the
following example.

     cp /usr/lib/python2.6/site-packages/shares/usbshare.py /usr/lib/python2.6/site-packages/shares/usbshare.py.old
     patch -i usbshare.py.SC.patch /usr/lib/python2.6/site-packages/shares/usbshare.py

### Optional - IPv6
The original Seagate v2.6.35 kernel did not support IPv6 at all
and so many components of the system are not, by default, configured 
to properly operate in an IPv6 environment.

If you would like to take full advantage of IPv6 functionality 
in the new kernel then a number of service startup and
configuration files need to be modified. If you would prefer 
to keep using only IPv4 as per the original Seagate Central
firmware then there's no need to complete the steps in this
section.

#### Bounce IPv6 on startup
The Seagate "networklan" daemon is in charge of monitoring the
Ethernet interface state and providing it's configuration. This
tool deliberately disables IPv6 on the ethernet interface as the
original Seagate Central firmware does not support IPv6 at all.

For this reason it is necessary to quickly turn IPv6 off then 
back on for the ethernet interface after the networklan daemon
is started.

Create a new script called "/etc/init.d/ipv6_bounce" by either 
copying it from the base directory of this project to the Seagate
Central, or by using an editor like "nano" or "vi". The script's
contents are as follows.

    #!/bin/sh
    KERNEL_VERSION=$(uname -r)
    KERNEL_VERSION_2=$(echo $KERNEL_VERSION | grep ^2\.)
    if [ ! -z $KERNEL_VERSION_2 ]; then
        exit 0
    fi
    
    # Disable then re-enable IPv6 on eth0
    sysctl -w net.ipv6.conf.eth0.disable_ipv6=1
    sysctl -w net.ipv6.conf.eth0.disable_ipv6=0
    
This script simply disables then re-enables IPv6 on the eth0
interface, but only if the new kernel is in operation.

Modify the permissions of the script to ensure it is executable.

     chmod 755 /etc/init.d/ipv6_bounce

Create a link to the script that causes it to be executed on system
bootup

    ln -s ../init.d/ipv6_bounce /etc/rcS.d/S44ipv6_bounce
    
Note that this script must be numerically ordered to execute after
the /etc/rcS.d/S41blackarmor-network startup script which starts the
networklan daemon.

#### Patch service configuration files for IPv6
In order for the services mentioned in this section to use IPv6
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
     
     # AVAHI - "Zero Conf" networking
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
to execute the steps in this section correctly. Please try to read the
explanation of what each step is trying to achieve and understand what 
you are doing before executing any of the following commands.

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

If the first copy of firmware is active then the output of the command will say

    current_kernel=kernel1
     
If the second copy of firmware is active then the output of the command will say

    current_kernel=kernel2

Take a note of which copy of firmware is currently active.

Interesting note: It is possible to manually change the value of this variable
in order to force the Seagate Central to switch to using the other copy of
firmware on reboot by using the "fw_setenv current_kernel kernel1" or 
"fw_setenv current_kernel kernel2" command. Keep this in mind if anything goes
particularly wrong!

#### Mount the kernel boot partition
The kernel image is kept on a disk partition which is not, by default, mounted
by the Seagate Central during normal operation. We will call this partition
the **kernel boot partition**.

If the **first** copy of firmware is active (kernel1) then the kernel boot 
partition is located on "/dev/sda1" . Mount the kernel boot partition with the
command

    mount /dev/sda1 /boot
     
If the **second** copy of firmware is active (kernel2) the the kernel boot 
partition is located on "/dev/sda2" . Mount the kernel boot partition with the
command

    mount /dev/sda2 /boot
     
#### Make a backup copy of the original kernel (uImage)
Change into the /boot directory where the currently active uImage kernel file
should be located and create a backup copy of the original kernel. 

    cp /boot/uImage /boot/uImage.old
     
#### Copy the new kernel into place
Copy the new kernel uImage into place so that on next boot it will be loaded
by the system. 

    cp uImage /boot/uImage

Confirm that the new uImage file is in place

    ls -l /boot
     
The output of the above command should show the new image and the backed up
original as per the following example (Note the file sizes may be slightly
different in your case)

    total 6729
    drwx------ 2 root root   12288 Nov 17  2015 lost+found
    -rw-r--r-- 1 root root 3857616 Sep  4 06:39 uImage
    -rw-r--r-- 1 root root 2989612 Sep  4 06:35 uImage.old

#### OPTIONAL - Copy the kernel modules into place
This step should only be performed if you have made your own custom changes
to the procedure in order to build kernel modules. By default, no kernel
modules need to be built or installed so this step can be skipped.

If you have an archive of kernel modules associated with the newly installed
uImage then extract it and check to make sure that the directory contents
are as expected.

     tar -xf cross-mod.tar.gz
     ls -laR cross-mod/

After checking that the modules have been extracted as expected, copy the
module tree into place as per the following example.

Note that this is a **very** dangerous part of the process so if you don't
understand what you are doing here then do not proceed with the following
command.

     cp -r cross-mod/lib /

There should be a new 5.x.x modules subdirectory on the unit alongside the 
modules subdirectory for the original v2.6.35 kernel. The output of the 
following command

     ls -l /lib/modules

should show an output similar to the following.

     drwxrwxr-x 4 root root 4096 Sep 17  2015 2.6.35.13-cavm1.whitney-econa.whitney-econa
     drwxr-xr-x 3 root root 4096 Sep  4 07:08 5.14.0-sc
     
Finally remove the original "modules.dep" file in the new module 
subdirectory. Removing this file will cause the unit to perform a
"depmod" for the newly installed modules on next boot which will
properly index them.

     rm /lib/modules/5.14.0-sc/modules.dep
     
### Reboot and confirm the upgrade    
Finally we reboot the unit and confirm that the new kernel is
operational.

Rebooting the unit can be performed via the Seagate Central command
line with the reboot command.

     reboot
     
Naturally, at the point when the unit is rebooted any ssh sessions to
the Seagate Central will be disconnected.

After the unit has rebooted re-establish an ssh connection to the newly
upgraded Seagate Central and issue the following command to confirm that
the unit has loaded the new kernel.

     uname -a
     
The output should indicate that the version of the running kernel is now
5.x.x and that SMP functionality is enabled, as per the following sample
output.

BERTO BERTO...SET THE SAME AS THE DOWNLOADABLE ONE
     Linux NAS-X 5.14.0-rc5-sc+ #138 SMP Wed Sep 8 14:54:33 AEST 2021 armv6l GNU/Linux       
     
Further confirm that the services you make use of on the Seagate Central
are functional, including

* The Web Management interface
* Samba / Windows file sharing 
* FTP/SFTP server
* DLNA media streaming service

### Optional - Revert back to the original kernel
If the new kernel version is not performing as desired then there is always 
the option of reinstating the original version.

If the procedure above has been followed then the sequence of
commands below issued with root priviledges on the Seagate Central 
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

Confirm that the original kernel is back in place by issuing the "uname -a"
command

     uname -a
     
The command output should indicate that kernel version has reverted back to
2.6.35 as per the following sample output.

    Linux NAS-X 2.6.35.13-cavm1.whitney-econa.whitney-econa #1 Wed Sep 16 15:47:59 PDT 2015 armv6l GNU/Linux

## Troubleshooting
The most problematic issue that may occur after a failed kernel upgrade
is that the unit is no longer accessible via ssh. If this happens then
the easiest route to pursue is to force the unit to revert to it's
backup firmware as per the procedure at

http://seagatecentralenhancementclub.blogspot.com/2015/08/revert-to-previous-firmware-on-seagate.html

Archive : https://archive.ph/3eOX0

In essence the steps are

1) Power down then power up the Seagate Central.
2) Wait 20 or so seconds for the LED status light on top of the unit to turn from amber to flashing green.
3) Execute first 2 steps again four times in a row.
5) Power up the unit. It should now attempt to load the backup / alternate version of firmware

If the unit is accessable via ssh then the best places to search for
troubleshooting data includes the system bootup log as displayed by
the **dmesg** command and the system log stored at /var/log/syslog

If some services are functional but others are not then check the 
log files pertinant to the failing services as well as relevant messages
in the the above mentioned log files.

* samba - /var/log/log.smbd  and /var/log/log.snmd
* FTP/SFTP - /var/log/vsftpd.log
* Web Management Interface - /var/log/user.log
