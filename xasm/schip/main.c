#include "xasm.h"
#include <stdlib.h>
#include <stdio.h>

static SConfiguration s_sConfiguration =
{
	"motorschip",
	"1.0",
	0x1000,
	ASM_BIG_ENDIAN,
	false,
	false,
	MINSIZE_8BIT,

	"rb", "rh", "rw",
	"db", "dh", "dw",
	"dsb", "dsh", "dsl"
};

SConfiguration* g_pConfiguration = &s_sConfiguration;

extern int main(int argc, char* argv[])
{
	return xasm_Main(argc, argv);
}
