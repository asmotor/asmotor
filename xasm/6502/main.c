#include "xasm.h"
#include <stdlib.h>
#include <stdio.h>

static SConfiguration s_sConfiguration =
{
	"motor6502",
	"1.0",
	0x10000,
	ASM_LITTLE_ENDIAN,
	false,
	false,
	MINSIZE_8BIT,
	1,

	"rb", "rw", "rl",
	"db", "dw", "dl",
	"ds", NULL, NULL
};

SConfiguration* g_pConfiguration = &s_sConfiguration;

extern int main(int argc, char* argv[])
{
	return xasm_Main(argc, argv);
}
