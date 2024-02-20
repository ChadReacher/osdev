#/bin/bash

sed -e 's/\s*\([\+0-9a-zA-Z]*\).*/\1/' << EOF | fdisk build/disk.img
	o # create a new empty DOS partition table
	n # new partition
	p # primary partition
	1 # partition number
	1 # first sector 
	2048 # last sector
	n # new partition
	p # primary partition
	2 # partition number
	2049 # first sector
	4096 # last sector
	n # new partition
	p # primary partition
	3 # partition number
	4097 # first sector
	6144 # last sector
	n # new partition
	p # primary partition
	4 # partition number
	6145 # first sector
	8191 # last sector
	p # print the partition table
	w # write the partition table
EOF
