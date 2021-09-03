# INSTRUCTIONS_MANUAL_KERNEL_INSTALLATION.md
This is a guide that describes how to manually replace the original
Linux v2.6.25 kernel on a Seagate Central NAS with a new, modern,
Linux v5.x kernel.

Refer to the README.md file for the location of a precompiled kernel.

Refer to the instructions in **INSTRUCTIONS_CROSS_COMPILE_KERNEL.md**
to self generate a kernel for use in this procedure.

Installation of the cross compiled kernel in conjunction with samba
by using the easier but less flexible firmware upgrade method
is covered by
BERTO **INSTRUCTIONS_FIRMWARE_UPGRADE_METHOD.md**

The target platform tested was a Seagate Central Single Drive NAS 
running firmware version 2015.0916.0008-F however I believe these 
instructions should work for other Seagate Central configurations 
and firmware versions as long as care is taken to account for any 
minor differences.

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
of troubeshooting problems during kernel boot.

### su/root access on the Seagate Central.
Make sure that you can establish an ssh session to the Seagate Central
and that you can succesfully issue the **su** command to gain root
priviledges. Note that some later versions of Seagate Central firmware
deliberately disable su access by default.

The alternative procedure detailed in BERTO
**INSTRUCTIONS_FIRMWARE_UPGRADE_METHOD.md** does not require su access
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
If kernel modules have been built then I would suggest archiving 
the directory they are stored in and then transferring the archive
to the Seagate Central as per the following example

     tar -caf cross-mod.tar.gz cross-mod/
     scp cross-mod admin@192.168.1.99:

Note that there are no modules included in the pre-built downloadable
kernel.

### Transfer the config and script patches to the Seagate Central
A number of configuration and script files need to be patched on
the Seagate Central in order to cater for the new kernel's requirements.

Patche files for these file are included in this project and have a
suffix of .SC.patch. These files need to be tranferred to the Seagate
Central. This can be done via scp as per the following example.

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

The script can be backed up and patched as follows.

     cp /usr/lib/python2.6/site-packages/shares/usbshare.py /usr/lib/python2.6/site-packages/shares/usbshare.py.old
     patch -i usbshare.py.SC.patch /usr/lib/python2.6/site-packages/shares/usbshare.py

### IPv6
If you would like to use the next generation IPv6 internet
protocol on the Seagate Central then a few configuration and
startup files need to be modified. 

The original Seagate v2.6.35 kernel did not support IPv6 at all
and so many components of the system are not by default configured 
to properly operate in an IPv6 environment.

#### Bounce IPv6 on startup
The Seagate "networklan" daemon is in charge of monitoring the
Ethernet interface state and providing it's configuration. This
tool, for some odd reason, deliberately disables IPv6 on the
ethernet interface. 

For this reason it is necessary to "bounce" IPv6 on the ethernet
interface after the networklan daemon is started.

Create a new script called "/etc/init.d/ipv6_bounce" using either
the nano or vi editor with the following contents

    #!/bin/sh
    KERNEL_VERSION=$(uname -r)
    KERNEL_VERSION_2=$(echo $KERNEL_VERSION | grep ^2\.)
    if [ ! -z $KERNEL_VERSION_2 ]; then
        exit 0
    fi
    
    # Disable then re-enable IPv6 on eth0
    sysctl -w net.ipv6.conf.eth0.disable_ipv6=1
    sysctl -w net.ipv6.conf.eth0.disable_ipv6=0
    
Modify the permissions of the script to ensure it is executable

     chmod 755 /etc/init.d/ipv6_bounce

Create a link to the script that causes it to be executed on system
bootup

    ln -s ../init.d/ipv6_bounce /etc/rcS.d/S44ipv6_bounce
    
Note that this script must be ordered to execute after the
S41blackarmor-network startup script.

#### Patch service configuration files for IPv6
A number of services need to be reconfigured to take full advantage
of IPv6. These configuration files can be backed up and patched 
as follows.

Note that only those services you wish to access via IPv6 need
to have their configuration patched

     cp /etc/netatalk/afpd.conf /etc/netatalk/afpd.conf.old
     patch -i afpd.conf.SC.patch /etc/netatalk/afpd.conf
     
     cp /etc/avahi/avahi-daemon.conf /etc/avahi/avahi-daemon.conf.old
     patch -i avahi-daemon.conf.SC.patch /etc/avahi/avahi-daemon.conf
     
     cp /etc/lighttpd.conf /etc/lighttpd.conf.old
     patch -i lighthttpd.conf.SC.patch /etc/lighttpd.conf
     
     cp /etc/vsftpd.conf /etc/vsftpd.conf.old
     patch -i vsftpd.conf.SC.patch /etc/vsftpd.conf
     
### Copy the new kernel into place
It's important to execute the steps in this section correctly. Please
try to read the explanation of what each step is trying to acheive and
understand what you are doing before executing any commands.

#### Find out which copy of firmware is active
The Seagate Central keeps two copies of firmware available on the
hard drive in case one copy becomes corrupted and cannot boot. In
this step we need to discover which of the two copies is currently
active on the Seagate Central so that we can modify that one.

The name of the currently active copy is stored in the flash memory
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
the **kernel partition**.

If the **first** copy of firmware is active then the kernel partition is located
on /dev/sda1 . Mount the kernel partition with the command

    mount /dev/sda1 /boot
     
If the **second** copy of firmware is active the the kernel partition is located
on /dev/sda2 . Mount the kernel partition with the command

    mount /dev/sda2 /boot
     
#### Make a backup copy of the original uImage
Change into the /boot directory where the currently active uImage kernel file
should be located and create a backup copy of the original kernel. 

    cd /boot
    cp uImage uImage.orig
     
Note that the kernel partition is only 20M in size, so while there will be
plenty of room to store the new kernel image and the backup, there won't be
room to store much more data.

#### Copy the new kernel into place
Copy the new kernel uImage into place so that on next boot it will be loaded
by the system. In this example the new uImage file is in the home directory of
the admin user and is copied into the boot directory.

    cp /Data/admin/uImage /boot/uImage

Confirm that the new uImage file is in place

    ls -l /boot
     
The output of the above command should show the new image and the backed up
original as per the following example (Note the file sizes may be slightly
different in your case)

    total 6729
    drwx------ 2 root root   12288 Nov 17  2015 lost+found
    -rw-r--r-- 1 root root 3857616 Sep  4 06:39 uImage
    -rw-r--r-- 1 root root 2989612 Sep  4 06:35 uImage.orig

#### OPTIONAL - Copy the kernel modules into place
If you have an archive of kernel modules associated with the newly installed
uImage then extract them into the directory where they have been placed then
copy them to the /lib directory on the unit.

     cd /Data/admin
     tar -xf cross-mod
     cd cross-mod/
     cp -r 



Copy the new kernel into place


OPTIONAL - Copy the modules into place

reboot


On bootup confirm that umame -a says the right thing


Optional = Revert back to the originals


Troubleshooting

If the old kernel is loaded with the new config files in place then
the ssh server will still work but the web management interface and
most other services will not operate.


Try the revert to the old firmware trick as per the URLs

Observe the status lights on the unit. They will be amber initially
then should switch to blinking green while the kernel is being loaded
then should switch to solid green once the unit has booted up.

After this point if you can't access the unit by ssh then something may
have gone wrong with the networking component. Try to "ping" the unit.
Make sure the status lights on the Ethernet LAN port are active.

The easiest thing to do at this point is to follow the document describing
how to force the Seagate Central to revert to the backup copy of firmware.

In essence the steps are

1) Power down the Seagate Central
2) After a few seconds power up the Seagate Central
3) Wait 15 or so seconds for the LED status light on top of the unit to start flashing green.
4) Execute steps 1 - 3 a total of four times in a row.
5) Power up the unit. It should now attempt to load the backup / alternate version of firmware

If the unit does not become accessible after this point then it might 
be necessary to take the next step by opening up the unit, removing the 
harddrive and mounting it on an external system.

I want to note that I have *never* had to do that during the course of
development and testing. The "reboot 4 times" method has always worked for 
me.


you have to physically power  reboot the 
Go to the section entitled
OPTIONAL - Reverting to the old kernel for more details on what to do.





