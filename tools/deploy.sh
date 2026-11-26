#!/bin/bash
set -e
PI=10.42.0.74
BIN=~/stm32-freertos-motor-monitor/firmware/stm32-freertos-motor-monitor/Debug/stm32-freertos-motor-monitor.bin
scp "$BIN" "$PI":~/safety_ecu.bin
ssh "$PI" "python3 ~/update_client.py ~/safety_ecu.bin"
