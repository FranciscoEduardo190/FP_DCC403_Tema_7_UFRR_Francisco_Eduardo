#!/bin/sh
set -eu

DEVICE_NODE=/dev/telemetry0

if [ -e "$DEVICE_NODE" ]; then
    sudo rm -f "$DEVICE_NODE"
fi

if lsmod | grep -q '^telemetry_driver'; then
    sudo rmmod telemetry_driver
fi

echo "telemetry_driver descarregado e $DEVICE_NODE removido"
