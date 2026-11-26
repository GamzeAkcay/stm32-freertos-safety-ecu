#!/bin/bash
set -e
PI=10.42.0.74
BIN=~/stm32-freertos-motor-monitor/firmware/stm32-freertos-motor-monitor/Debug/stm32-freertos-motor-monitor.bin
ssh "$PI" "ip link show can0 | grep -q 'state UP'" || { echo "HATA: Pi'de can0 kapali. Once ac: sudo ip link set can0 up type can bitrate 500000"; exit 1; }
scp "$BIN" "$PI":~/safety_ecu.bin
ssh "$PI" "python3 ~/update_client.py ~/safety_ecu.bin"
