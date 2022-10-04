#!/bin/sh
KERNEL_VERSION=$(uname -r)
KERNEL_VERSION_2=$(echo $KERNEL_VERSION | grep ^2\.)
if [ ! -z $KERNEL_VERSION_2 ]; then
    exit 0
fi
    
# Disable then re-enable IPv6 on eth0
sysctl -w net.ipv6.conf.eth0.disable_ipv6=1
sysctl -w net.ipv6.conf.eth0.disable_ipv6=0
