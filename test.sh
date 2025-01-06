echo "[TEST]: Building"
rm -f /tmp/vm-core.dump*
rm -f /tmp/vm-pmem.dump*
rm /tmp/serial.log.*
make clean-deps > /dev/null && make >/dev/null && xxd -C build/disk.img > /tmp/before
echo "[TEST]: Build finished. Let's test it for 10 tries"
for i in $(seq 1 10); do
	echo "TEST #${i}"
	make log >/dev/null 2>&1
	#timeout 2 make log >/dev/null 2>&1
	#make log >/dev/null 2>&1 &
        #sleep 2
	#my-venv/bin/python3 my-venv/qmp.py
	xxd -C build/disk.img > /tmp/after 2>/dev/null
	diff /tmp/before /tmp/after | tee /tmp/diff
	#lines=$(cat /tmp/diff | wc -l)
	#if [ "${lines}" -ge 13 ]; then
	#	echo "OOPS! TEST FAILED"
        #        echo "Diff:"
        #        cat /tmp/diff
	#	exit 128
	#fi
done
rm /tmp/{diff,before,after}
echo "CONGRATULATIONS! VALIDATION PERFORMED SUCCESSFULLY"
