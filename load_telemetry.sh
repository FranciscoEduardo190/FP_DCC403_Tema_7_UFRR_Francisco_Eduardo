#!/bin/sh
set -eu

MODULE=telemetry_driver.ko
DEVICE_NAME=telemetry
DEVICE_NODE=/dev/telemetry0

if ! lsmod | grep -q '^telemetry_driver'; then
    sudo insmod "$MODULE"
fi

MAJOR=$(awk -v name="$DEVICE_NAME" '$2 == name { print $1 }' /proc/devices)

if [ -z "$MAJOR" ]; then
    echo "Nao foi possivel encontrar o major number de $DEVICE_NAME em /proc/devices" >&2
    exit 1
fi

if [ -e "$DEVICE_NODE" ]; then
    sudo rm -f "$DEVICE_NODE"
fi

sudo mknod "$DEVICE_NODE" c "$MAJOR" 0
sudo chmod 666 "$DEVICE_NODE"

echo "$DEVICE_NODE criado com major=$MAJOR minor=0"
