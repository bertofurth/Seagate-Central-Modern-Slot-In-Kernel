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

The alternative procedure detailed in BERTO
**INSTRUCTIONS_FIRMWARE_UPGRADE_METHOD.md** does not require su access
and will in fact automatically re-enable su access as a result of the
procedure.

### Know how to copy files between your host and the Seagate Central. 
Not only should you know how to transfer files to and from your Seagate
Central NAS and the build host, ideally you'll know how to transfer files 
**even if the samba service is not working**. I would suggest
that if samba is not working to use FTP or SCP which should both still work.

### Samba version on the Seagate Central
Although this is not strictly a pre-requisite of this kernel installation
procedure it is worth re-emphazing here that the original samba file server
software on the Seagate Central will not work after the new kernel is
installed on the Seagate Central.

See the README.md file in this project and the Seagate-Central-Samba 
project at the following link for more details and instructions on how to
upgrade the samba service on the Seagate Central.

https://github.com/bertofurth/Seagate-Central-Samba


## Procedure
### Transfer the cross compiled kernel to the Seagate Central
You should have a self generated or downloaded kernel "uImage" file
which you'd like to install on the Seagate Central. 

If you have just completed cross compiling the kernel as per the 
instructions in this project then relative to the base working 
directory the generated uImage file will be located at

obj/arch/arm/boot/uImage

Transfer this image to the Seagate Central. In this example we use 
the scp command with the "admin" user to copy the kernel to the 
user's home directory. You will need to substitute your own username
and NAS IP address. After executing the scp command you'll be prompted
for the user's password.

    scp obj/arch/arm/boot/uImage admin@<NAS-ip-address>:
    
### Optional - Transfer kernel modules to the Seagate Central
If kernel modules have been built then I would suggest archiving 
the directory they are stored in and then transferring the archive
to the Seagate Central as per the following example

     tar -caf cross-mod.tar.gz cross-mod
     scp cross-mod admin@<NAS-ip-address>:

#### Transfer the config and script patches to the Seagate Central
A number of configuration and script files need to be patched on
the Seagate Central in order to cater for the new kernel's requirements.

Patches for these files have a suffix of .SC.patch and need to be
tranferred to the Seagate Central. This can be done via scp as per the
following example.

     scp *.SC.patch admin<NAS-ip-address>:
     
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

Create a new script called "/etc/init.d/ipv6_bounce" using either the
nano or vi editor with the following contents

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




Copy the new kernel into place

OPTIONAL - Copy the modules into place

reboot


On bootup confirm that umame -a says the right thing


Optional = Revert back to the originals


Troubleshooting

If the old kernel is loaded with the new config files in place then
the ssh server will still work but the web management interface and
most other services will not operate.


