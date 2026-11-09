#!/bin/bash
# Double-click to check that the Mac can open Mishkal Wall (Audio Unit validation)
cd "$(dirname "$0")"
LOG="$PWD/Wall-check.log"
exec > >(tee "$LOG") 2>&1
echo "=== Wall check  $(date) ==="
killall -9 AudioComponentRegistrar >/dev/null 2>&1
auval -v aumf Wall Mshk > Wall-auval-full.log 2>&1; grep -nE "ERROR|FAIL|WARNING" Wall-auval-full.log | head -30; tail -6 Wall-auval-full.log
echo "=== CHECK DONE ==="
read -p "Press Return to close."
