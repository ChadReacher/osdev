#!/bin/bash

if [[ $EUID > 0 ]]
	then echo "Run this script as root"
	exit
fi

BRIDGE="br0"
TAP="tap0"
INTERFACE="enp0s3"

echo "Adding bridge $BRIDGE"
ip link add name $BRIDGE type bridge

echo "Flushing interface $INTERFACE"
ip addr flush dev $INTERFACE

echo "Setting $BRIDGE as master of $INTERFACE"
ip link set $INTERFACE master $BRIDGE

echo "Adding tap $TAP"
ip tuntap add $TAP mode tap

echo "Setting $BRIDGE as master of $TAP"
ip link set $TAP master $BRIDGE

echo "Setting $INTERFACE, $BRIDGE and $TAP up"
ip link set up dev $INTERFACE
ip link set up dev $TAP
ip link set up dev $BRIDGE

echo "Requesting ip for $BRIDGE"
ip addr add 192.168.0.115/24 dev $BRIDGE
ip route add default via 192.168.0.1 dev $BRIDGE
