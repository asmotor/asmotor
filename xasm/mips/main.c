#include "xasm.h"
#include <stdlib.h>
#include <stdio.h>

static SConfiguration s_sConfiguration =
{
	"motormips",
	"1.0",
	0x7FFFFFFF,
	ASM_LITTLE_ENDIAN,
	false,
	false,
	MINSIZE_8BIT,
	8,

	"rb", "rh", "rw",
	"db", "dh", "dw",
	"dsb", "dsh", "dsl"
};

SConfiguration* g_pConfiguration = &s_sConfiguration;

extern int main(int argc, char* argv[])
{
	return xasm_Main(argc, argv);
}
