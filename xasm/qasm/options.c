#include <stddef.h>

#include "mem.h"

#include "options.h"
#include "qasm.h"

SOptions* opt_Current = NULL;


static SOptions*
createOptions(void) {
	SOptions* options = (SOptions*) mem_Alloc(sizeof(SOptions));

	options->next = NULL;
	options->machineOptions = qasm_Configuration->allocOptions();
	options->endianness = qasm_Configuration->defaultEndianness;

	return options;
}


extern void
opt_Init(void) {
	opt_Current = createOptions();
}


extern void
opt_Close(void) {
	SOptions* options = opt_Current;
	while (options) {
		SOptions* next = options->next;
		mem_Free(options->machineOptions);
		mem_Free(options);
		options = next;
	}
}