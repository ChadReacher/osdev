#/bin/bash

sed -e 's/\s*\([\+0-9a-zA-Z]*\).*/\1/' << EOF | fdisk /dev/loop0
	n # new partition
	p # primary partition
	1 # partition number
	1 # first sector (default)
	4096  # last sector (default)
	n # new partition
	p # primary partition
	2 # partition number
	  # first sector (default)
	  # last sector (default)
	p # print the partition table
	w # write the partition table
EOF
