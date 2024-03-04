#!/bin/bash
max=100
for i in `seq 1 $max`
do
	touch "userland/hdd/somefile${i}filewithverylongname${i}"
done
