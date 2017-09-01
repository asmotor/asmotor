#include "xasm.h"
#include <stdlib.h>
#include <stdio.h>

static SConfiguration s_sConfiguration =
{
	"motor0x10c",
	"1.0",
	0x10000,
	ASM_BIG_ENDIAN,
	false,
	false,
	MINSIZE_16BIT,
	2,
	
	NULL, "rw", "rl",
	NULL, "dw", "dl",
	NULL, "dw", NULL
};

SConfiguration* g_pConfiguration = &s_sConfiguration;

extern int main(int argc, char* argv[])
{
	return xasm_Main(argc, argv);
}
