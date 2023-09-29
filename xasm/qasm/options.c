#include <stddef.h>

#include "options.h"
#include "qasm.h"

SOptions* opt_Current = NULL;


static SOptions*
createOptions(void) {
	SOptions* options = (SOptions*) malloc(sizeof(SOptions));

	options->next = NULL;
	options->prev = NULL;
	options->machineOptions = qasm_Configuration->allocOptions();
	qasm_Configuration->setDefaultOptions(options->machineOptions);

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
		free(options->machineOptions);
		free(options);
		options = next;
	}
}