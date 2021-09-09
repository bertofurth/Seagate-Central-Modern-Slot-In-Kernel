#!/bin/bash

# Check system state and update the status LED
# on the top of the Seagate Central accordingly.

# States are indicated with the following precedence
#
# Loss of connectivity to Default router - Flash Red (5)
# Sustained High CPU - Flash Red/Green (7)
# Sustained Moderate CPU - Slow Flash Red/Green (11)
# Low disk space on main user data volume - Slow Flash Amber (10)
# Normal - Solid Green (1)

# Install the script as follows.
#
# Copy the script to /usr/bin then make
# sure the script is executable by running
#
# chmod 755 /usr/bin/new-led-control.sh
#
# Add or modify a startup script to run this script
# as a daemon in the backgroud.
#
# For example, modify the following startup script 
#
# /etc/init.d/ledsmanager
#
# and add a line at the end that will execute
# the script in the background.
#
# /usr/bin/new-led-control.sh &
#

# Set the following thresholds for "high" and "moderate"
# cpu and the disk too full percentage

high=1.9
moderate=1.5
disk_full=97

while :
do
        # Sleep for 10 seconds
	sleep 10

	# Default gw connectivity
	default_gw=$(route -n | grep UG | cut -b17-32)
	ping -q -c1 -w1 $default_gw  &> /dev/null
	if [ ! $? -eq 0 ]; then
        	# Try once more
        	ping -q -c1 -w1 $default_gw  &> /dev/null
        	if [ ! $? -eq 0 ]; then
			echo 5 > /proc/cns3xxx/leds
			continue
		fi
	fi 

	# High CPU Usage
	loadavg=$(cat /proc/loadavg | awk '//{print $1}')
	if [ 1 -eq $(echo "$loadavg > $high" | bc) ]; then
		echo 7 > /proc/cns3xxx/leds
		continue
	fi

	# Moderate CPU Usage 
	if [ 1 -eq $(echo "$loadavg > $moderate" | bc) ]; then
		echo 11 > /proc/cns3xxx/leds
		continue
	fi


	# Data patition usage
	disk_usage_percent=$(df /Data | awk '//{print $5}')
	disk_usage=${disk_usage_percent%?}
	if [ 1 -eq $(echo "$disk_usage > $disk_full" | bc) ] ; then
		echo 10 > /proc/cns3xxx/leds
        	continue
	fi

	# Everything is ok
	echo 1 > /proc/cns3xxx/leds	
done
