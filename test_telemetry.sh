#!/bin/sh
set -eu

DEVICE_NODE=/dev/telemetry0

if [ ! -e "$DEVICE_NODE" ]; then
    echo "$DEVICE_NODE nao existe. Execute ./load_telemetry.sh primeiro." >&2
    exit 1
fi

echo "EMERGENCY_STOP=1" > "$DEVICE_NODE"
head -n 3 "$DEVICE_NODE"

echo "RESET" > "$DEVICE_NODE"

for worker in 1 2 3; do
    (head -n 4 "$DEVICE_NODE" > "/tmp/telemetry_worker_${worker}.log") &
done
wait

cat /tmp/telemetry_worker_*.log
rm -f /tmp/telemetry_worker_*.log

dmesg | tail -n 20
