#!/bin/bash
# Regression test for command-line option parsing: an option argument may be
# attached to the option letter ("-ofile") or given as a separate command-line
# token ("-o file"). Both forms must produce identical results, and an option
# that requires an argument must report an error when none is given. The -D
# option is also exercised with a value ("-D NAME=value", $hex and %binary).

BIN=../../build/cmake/debug
ASM="$BIN/xasm/rc8/motorrc8"
LNK="$BIN/xlink/xlink"

rm -f spaced.obj spaced.bin spaced.map attached.obj attached.bin attached.map

"$ASM" -g -e l -D 'CLITEST=$2A' -D FLAGDEF -o spaced.obj src.rc8 >/dev/null || { echo "FAIL: assemble (spaced)"; exit 1; }
"$LNK" -c hc800b -f hc800k -s Start -m spaced.map -o spaced.bin spaced.obj >/dev/null || { echo "FAIL: link (spaced)"; exit 1; }

"$ASM" -g -el -DCLITEST=%101010 -DFLAGDEF -oattached.obj src.rc8 >/dev/null || { echo "FAIL: assemble (attached)"; exit 1; }
"$LNK" -chc800b -fhc800k -sStart -mattached.map -oattached.bin attached.obj >/dev/null || { echo "FAIL: link (attached)"; exit 1; }

if ! cmp -s spaced.bin attached.bin || ! cmp -s spaced.map attached.map; then
    echo "FAIL: spaced option arguments produce different output than attached ones"
    exit 1
fi

# Every argument-taking assembler option must accept both spellings and yield
# identical output.
check_pair() {
    rm -f p1.obj p2.obj chain.obj dup.obj srcsym.rc8 srcsym.obj usecmd.rc8 usecmd.obj redefcmd.rc8 redef1.obj setcmd.rc8 redef2.obj
    "$ASM" $1 -D CLITEST=42 -D FLAGDEF -o p1.obj src.rc8 >/dev/null || { echo "FAIL: spaced form rejected: $1"; exit 1; }
    "$ASM" $2 -DCLITEST=42 -DFLAGDEF -op2.obj src.rc8 >/dev/null || { echo "FAIL: attached form rejected: $2"; exit 1; }
    cmp -s p1.obj p2.obj || { echo "FAIL: $1 differs from $2"; exit 1; }
    rm -f p1.obj p2.obj chain.obj dup.obj srcsym.rc8 srcsym.obj usecmd.rc8 usecmd.obj redefcmd.rc8 redef1.obj setcmd.rc8 redef2.obj
}
check_pair "-a 16" "-a16"
check_pair "-b 01" "-b01"
check_pair "-e l" "-el"
check_pair "-i ." "-i."
check_pair "-w 0001" "-w0001"
check_pair "-z FF" "-zFF"
check_pair "-m s1" "-ms1"

# A missing argument must be reported for every option that requires one
# (the option is last, so there is no token left to swallow).
for opt in -o -d -s -D -a -b -e -f -w -z; do
    out=$("$ASM" -D CLITEST=42 -D FLAGDEF "$opt" 2>&1)
    echo "$out" | grep -q "Error in option" || { echo "FAIL: missing argument for $opt not reported"; exit 1; }
done

# -D values go through the constant-expression parser.
printf '\tSECTION "Code",CODE\n\tIF A==5 && B==A+1 && CLITEST==42\n\tELSE\n\tFAIL "expr"\n\tENDC\nStart:\n\tld ft,$00\n' > exprchk.rc8
"$ASM" -D A=5 -D 'B=A+1' -D 'CLITEST=6*7' -D FLAGDEF -o expr.obj exprchk.rc8 >/dev/null || { echo "FAIL: -D expressions rejected"; exit 1; }
if "$ASM" -D 'CLITEST=oops' -D FLAGDEF -o bad.obj src.rc8 >/dev/null 2>&1; then
    echo "FAIL: bad -D value accepted"
    exit 1
fi

# -D values may refer to earlier -D defines and to built-in symbols, but not
# to symbols defined later in the source (the file has not been parsed yet).
"$ASM" -D CLITEST=42 -D 'C=$2A' -D 'D=C+1' -D 'E=D*2' -D 'F=__ASMOTOR' -D FLAGDEF -o chain.obj src.rc8 >/dev/null || { echo "FAIL: chained -D references rejected"; exit 1; }
if "$ASM" -D C=1 -D C=2 -D FLAGDEF -o dup.obj src.rc8 >/dev/null 2>&1; then
    echo "FAIL: duplicate -D define not reported"
    exit 1
fi
# A command-line symbol can be used by the source, but not redefined by it.
printf '\tSECTION "Code",CODE\nStart:\n\tld\tft,CMDLINEVAL\n' > usecmd.rc8
"$ASM" -D CMDLINEVAL=42 -o usecmd.obj usecmd.rc8 >/dev/null || { echo "FAIL: source cannot use -D symbol"; exit 1; }
printf '\tSECTION "Code",CODE\nCMDLINEVAL\tEQU\t7\nStart:\n\tld\tft,CMDLINEVAL\n' > redefcmd.rc8
if "$ASM" -D CMDLINEVAL=42 -o redef1.obj redefcmd.rc8 >/dev/null 2>&1; then
    echo "FAIL: source EQU redefinition of -D symbol accepted"
    exit 1
fi
printf '\tSECTION "Code",CODE\nCMDLINEVAL\tSET\t7\nStart:\n\tld\tft,CMDLINEVAL\n' > setcmd.rc8
if "$ASM" -D CMDLINEVAL=42 -o redef2.obj setcmd.rc8 >/dev/null 2>&1; then
    echo "FAIL: source SET of -D symbol accepted"
    exit 1
fi

printf '\tSECTION "Code",CODE\nEQUVAL\tEQU\t7\n\tIF CLITEST==42\n\tELSE\n\tFAIL "x"\n\tENDC\nStart:\n\tld ft,$00\n' > srcsym.rc8
if "$ASM" -D 'CLITEST=EQUVAL' -D FLAGDEF -o srcsym.obj srcsym.rc8 >/dev/null 2>&1; then
    echo "FAIL: -D silently accepted a source symbol"
    exit 1
fi

# Linker: slash option prefix, entry point and listfile in spaced form.
"$LNK" /c hc800b /f hc800k /e Start /s Start /l slash.lst /m slash.map /o slash.bin spaced.obj >/dev/null || { echo "FAIL: slash-prefixed options rejected"; exit 1; }
cmp -s slash.bin spaced.bin || { echo "FAIL: slash-prefixed options differ"; exit 1; }

"$LNK" -Z -c hc800b -f hc800k -s Start -m unknown.map -o unknown.bin spaced.obj >/dev/null 2>&1 && { echo "FAIL: unknown linker option accepted"; exit 1; }

# Help text is provided by the shared command-line layer.
"$ASM" -h 2>/dev/null | grep -q "Usage:" || { echo "FAIL: assembler help missing"; exit 1; }
"$LNK" -h 2>/dev/null | grep -q "Usage:" || { echo "FAIL: linker help missing"; exit 1; }

rm -f spaced.obj spaced.bin spaced.map attached.obj attached.bin attached.map missing.map
rm -f p1.obj p2.obj chain.obj dup.obj srcsym.rc8 srcsym.obj usecmd.rc8 usecmd.obj redefcmd.rc8 redef1.obj setcmd.rc8 redef2.obj exprchk.rc8 expr.obj bad.obj slash.lst slash.map slash.bin unknown.map unknown.bin
exit 0
