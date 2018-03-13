#ifndef __lf_dyld_h__
#define __lf_dyld_h__

#include <flipper/fmr.h>

struct _lf_module *dyld_register(struct _lf_device *device, char *module, void **jumptable);
int dyld_load(struct _lf_device *device, void *src, size_t len);
struct _lf_module *dyld_module(struct _lf_device *device, char *module);
int dyld_unload(struct _lf_device *device, char *module);

#endif
