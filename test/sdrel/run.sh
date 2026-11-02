#!/bin/bash
# Regression tests for the SDCC/sdas RREL (.rel) object reader.
#
# The expected output is the Intel-hex image produced by the reference
# linker (sdcc's sdldrc8) for the same objects; the .sdld.ihx fixtures are
# captured oracle output, and the comparison checks every byte sdld emits
# against xlink's flat binary at the same address. xlink additionally
# zero-pads reserved (declared but unwritten) area space, which the hex
# file omits, so bytes beyond the hex image must be zero.
#
# Cases:
#  - libc.rel: single module, byte/word/PC-relative relocations, area and
#    symbol references, BYTX padding removal
#  - hello.rel + muldiv.rel: two modules merged per area, cross-module
#    symbol references, synthesized s_/l_ area-label symbols, reserved
#    data space
#  - badmagic-*.rel, mode05.rel, pline.rel: unsupported constructs are
#    refused with an error instead of being guessed at

BIN=../../build/cmake/debug
XLINK="$BIN/xlink/xlink"
status=0

compare() {
    oracle=$1; shift
    rm -f out.bin
    "$XLINK" -aflat.mem -fbin -o out.bin "$@" || { echo "FAIL: link $*"; return 1; }
    python3 - "$oracle" out.bin <<'EOF'
import sys
def ihx(path):
    img = {}; ext = 0
    for line in open(path, errors='replace'):
        if not line.startswith(':'):
            continue
        n = int(line[1:3], 16); a = int(line[3:7], 16); t = int(line[7:9], 16)
        d = line[9:9 + 2 * n]
        if t == 0:
            for i in range(n):
                img[ext + a + i] = int(d[2 * i:2 * i + 2], 16)
        elif t == 5:
            ext = int(d, 16) << 16
        elif t == 1:
            break
    return img
img = ihx(sys.argv[1])
out = open(sys.argv[2], 'rb').read()
bad = [(k, v, out[k] if k < len(out) else None) for k, v in sorted(img.items())
       if k >= len(out) or out[k] != v]
if bad:
    print('FAIL: %d mismatching bytes, first: %s' % (len(bad), ['%#x oracle=%#x xlink=%s' % (k, v, '-' if x is None else hex(x)) for k, v, x in bad[:5]]))
    sys.exit(1)
EOF
    [ $? -eq 0 ] || return 1
    rm -f out.bin
    return 0
}

refuse() {
    file=$1; what=$2; shift 2
    rm -f out.bin
    output=$("$XLINK" -aflat.mem -fbin -o out.bin "$file" 2>&1)
    rc=$?
    rm -f out.bin
    if [ $rc -eq 0 ]; then
        echo "FAIL: $file linked but must be refused ($what)"
        return 1
    fi
    if ! echo "$output" | grep -q "$what"; then
        echo "FAIL: $file refused for the wrong reason: $output"
        return 1
    fi
    return 0
}

compare libc.sdld.ihx libc.rel || status=1
compare hm.sdld.ihx hello.rel muldiv.rel || status=1

refuse badmagic-dl2.rel "unsupported RREL magic" || status=1
refuse badmagic-dh2.rel "unsupported RREL magic" || status=1
refuse mode05.rel "unsupported relocation mode" || status=1
refuse pline.rel "paging" || status=1

exit $status
