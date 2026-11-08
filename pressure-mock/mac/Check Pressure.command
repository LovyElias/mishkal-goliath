#!/bin/bash
# Double-click to check that the Mac can open Mishkal Pressure (Audio Unit validation)
cd "$(dirname "$0")"
LOG="$PWD/Pressure-check.log"
exec > >(tee "$LOG") 2>&1
echo "=== Pressure check  $(date) ==="
killall -9 AudioComponentRegistrar >/dev/null 2>&1
auval -v aufx Prss Mshk > Pressure-auval-full.log 2>&1; grep -nE "ERROR|FAIL|WARNING" Pressure-auval-full.log | head -30; tail -6 Pressure-auval-full.log
echo "=== CHECK DONE ==="
read -p "Press Return to close."
