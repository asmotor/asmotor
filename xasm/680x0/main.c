#include "xasm.h"
#include <stdlib.h>
#include <stdio.h>

static SConfiguration s_sConfiguration =
{
	"motor68k",
	"1.0",
	0x7FFFFFFF,
	ASM_BIG_ENDIAN,

	"rs.b", "rs.w", "rs.l",
	"dc.b", "dc.w", "dc.l",
	"ds.b", "ds.w", "ds.l"
};

extern SConfiguration* g_pConfiguration = &s_sConfiguration;

extern int main(int argc, char* argv[])
{
	return xasm_Main(argc, argv);
}
