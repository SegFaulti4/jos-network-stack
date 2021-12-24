#!/bin/bash
jobs=()
trap 'kill $jobs' EXIT HUP TERM INT
make -s udp_send_recv
NETWORK_ENABLE=1 make -C .. qemu 2>/dev/null 1>out.txt & jobs+=($!)
sleep 20
python3 tests.py
if cat out.txt | grep -q "HELLO"; then echo "JOS UDP output - OK"; else echo "JOS UDP output - FAIL"; fi
rm out.txt
