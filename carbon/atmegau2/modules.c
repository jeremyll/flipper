#define __private_include__
#include <flipper/carbon.h>

/* Define the standard module array for this platform. */
const void *const fmr_modules[] = {
    &button,
    &fs,
	&gpio,
    &led,
    &uart0,
    &wdt
};