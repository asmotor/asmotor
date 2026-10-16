#!/bin/bash
# Regression test for pinned-section retention.
#
# A fixed-origin BSS[0] section whose symbols are referenced only through
# cross-section constant uses (which the assembler folds to immediates,
# producing no relocation) must still be kept by the linker, so the following
# free BSS section is placed after it. If the pinned section is dropped, the
# free section overlaps it and "sda" lands at $0000 instead of $0002.

BIN=../../build/cmake/debug
rm -f pinnedbss.obj pinnedbss.map pinnedbss.bin

"$BIN/xasm/rc8/motorrc8" -g -el -opinnedbss.obj pinnedbss.rc8 || { echo "FAIL: assemble"; exit 1; }
"$BIN/xlink/xlink" -chc800b -fhc800k -sStart -mpinnedbss.map -opinnedbss.bin pinnedbss.obj || { echo "FAIL: link"; exit 1; }

if grep -q '^\$0000 | freeSectors' pinnedbss.map && grep -q '^\$0002 | sda' pinnedbss.map; then
    rm -f pinnedbss.obj pinnedbss.map pinnedbss.bin
    exit 0
fi

echo "FAIL: pinned BSS section not retained (expected freeSectors@\$0000, sda@\$0002)"
exit 1
