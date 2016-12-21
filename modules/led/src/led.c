#define __private_include__
#include <flipper/led.h>

#ifdef __use_led__
/* Define the virtual interface for this module. */
const struct _led led = {
	led_configure,
	led_set_rgb
};
#endif
