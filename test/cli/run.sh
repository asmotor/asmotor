#!/bin/bash
# Regression test for command-line option parsing: an option argument may be
# attached to the option letter ("-ofile") or given as a separate command-line
# token ("-o file"). Both forms must produce identical results, and an option
# that requires an argument must report an error when none is given.

BIN=../../build/cmake/debug
rm -f spaced.obj spaced.bin spaced.map attached.obj attached.bin attached.map

"$BIN/xasm/rc8/motorrc8" -g -e l -o spaced.obj src.rc8 || { echo "FAIL: assemble (spaced)"; exit 1; }
"$BIN/xlink/xlink" -c hc800b -f hc800k -s Start -m spaced.map -o spaced.bin spaced.obj || { echo "FAIL: link (spaced)"; exit 1; }

"$BIN/xasm/rc8/motorrc8" -g -el -oattached.obj src.rc8 || { echo "FAIL: assemble (attached)"; exit 1; }
"$BIN/xlink/xlink" -chc800b -fhc800k -sStart -mattached.map -oattached.bin attached.obj || { echo "FAIL: link (attached)"; exit 1; }

if ! cmp -s spaced.bin attached.bin || ! cmp -s spaced.map attached.map; then
    echo "FAIL: spaced option arguments produce different output than attached ones"
    exit 1
fi

if "$BIN/xlink/xlink" -c hc800b -f hc800k -s Start -m missing.map -o >/dev/null 2>&1; then
    echo "FAIL: missing option argument not reported"
    rm -f missing.map
    exit 1
fi

rm -f spaced.obj spaced.bin spaced.map attached.obj attached.bin attached.map missing.map
exit 0
