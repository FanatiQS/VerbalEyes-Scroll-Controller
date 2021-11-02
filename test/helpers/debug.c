#include <stdio.h>

#include "./print_colors.h"

int numberOfErrors = 0;

void debug_printerrors() {
	printf("\n\n" COLOR_BLUE "RESULT:\n" COLOR_NORMAL);
	printf((numberOfErrors) ? COLOR_RED : COLOR_GREEN);
	printf("Number of errors: %d\n", numberOfErrors);
	printf("" COLOR_NORMAL);
}
