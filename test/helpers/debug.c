#include <stdio.h>
#include <stdlib.h>

#include "./print_colors.h"

int numberOfErrors = 0;

int debug_printerrors() {
	printf("\n\n" COLOR_BLUE "RESULT:\n" COLOR_NORMAL);
	printf((numberOfErrors) ? COLOR_RED : COLOR_GREEN);
	printf("Number of errors: %d\n", numberOfErrors);
	printf("" COLOR_NORMAL);
	return (numberOfErrors) ? EXIT_FAILURE : EXIT_SUCCESS;
}
