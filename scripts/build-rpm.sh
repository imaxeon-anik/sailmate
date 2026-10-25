#!/bin/bash
set -euo pipefail
ARCH="${1:-aarch64}"
case "$ARCH" in aarch64|armv7hl|i486) ;; *) echo 'Usage: build-rpm.sh [aarch64|armv7hl|i486]' >&2; exit 2 ;; esac
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SSD_ROOT="$(cd "$ROOT/../.." && pwd)"
OUT="$SSD_ROOT/xperia10v/app-rpms"
ssh -o BatchMode=yes -o ConnectTimeout=5 sfbuild true
rsync -a --delete --exclude .git --exclude RPMS --exclude build --exclude '*.o' \
  --exclude Makefile --exclude '.qmake.stash' --exclude 'moc_*' --exclude 'qrc_*' \
  "$ROOT/" sfbuild:~/src/sailmate/
ssh sfbuild "cd ~/src/sailmate && rm -rf RPMS && ~/SailfishOS/bin/sfdk -c target=SailfishOS-5.1.0.11-$ARCH build"
mkdir -p "$OUT"
rsync -a "sfbuild:~/src/sailmate/RPMS/*.$ARCH.rpm" "$OUT/"
ls -lh "$OUT"/harbour-sailmate-*.$ARCH.rpm
