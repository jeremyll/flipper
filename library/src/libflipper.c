#include <flipper.h>

lf_device_list lf_attached_devices;
struct _lf_device *lf_current_device;
lf_event_list lf_registered_events;

/* Creates a new libflipper device. */
struct _lf_device *lf_device_create(struct _lf_endpoint *endpoint, int (* select)(struct _lf_device *device), int (* destroy)(struct _lf_device *device), size_t context_size) {
	struct _lf_device *device = (struct _lf_device *)calloc(1, sizeof(struct _lf_device));
	lf_assert(device, failure, E_MALLOC, "Failed to allocate memory for new device.");
	device->endpoint = endpoint;
	device->select = select;
	device->destroy = destroy;
	device->_ctx = calloc(1, context_size);
	return device;
failure:
	free(device);
	return NULL;
}

void lf_set_current_device(struct _lf_device *device) {
	lf_current_device = device;
}

struct _lf_device *lf_get_current_device(void) {
	return lf_current_device;
}

int lf_device_release(struct _lf_device *device) {
	if (device) {
		lf_endpoint_release(device->endpoint);
		if (device->destroy) device->destroy(device);
		free(device->_ctx);
		free(device);
	}
	return lf_success;
}

/* Attempts to attach to all unattached devices. Returns how many devices were attached. */
int lf_attach(struct _lf_device *device) {
	lf_assert(device, failure, E_NULL, "Attempt to attach an invalid device.");
	lf_ll_append(&lf_attached_devices, device, lf_device_release);
	lf_select(device);
	return lf_success;
failure:
	return lf_error;
}

/* Call's the device's selector function and selects the device. */
int lf_select(struct _lf_device *device) {
	lf_assert(device, failure, E_NULL, "NULL device pointer provided for selection.");
	device->select(device);
	lf_set_current_device(device);
failure:
	return lf_error;
}

/* Detaches a device from libflipper. */
int lf_detach(struct _lf_device *device) {
	lf_assert(device, failure, E_NULL, "Invalid device provided to detach.");
	lf_ll_remove(&lf_attached_devices, device);
	return lf_success;
failure:
	return lf_error;
}

/* Deactivates libflipper state and releases the event loop. */
int __attribute__((__destructor__)) lf_exit(void) {
	/* Release all of the libflipper events. */
	lf_ll_release(&lf_get_event_list());
	/* Release all of the attached devices. */
	lf_ll_release(&lf_attached_devices);
	return lf_success;
}

/* Binds the lf_module structure to its counterpart on the attached device. */
LF_WEAK int lf_bind(struct _lf_module *module, struct _lf_device *device) {
	lf_assert(module, failure, E_MODULE, "NULL module passed to '%s'.", __PRETTY_FUNCTION__);
	lf_assert(device, failure, E_NULL, "NULL device passed to '%s'.", __PRETTY_FUNCTION__)
	lf_assert(module->name, failure, E_MODULE, "Module has no name.");
	lf_debug("Binding to module '%s'.", module->name);
	module->device = device;
	module->identifier = lf_crc(module->name, strlen(module->name) + 1);
	int index = fld_index(module->identifier);
	if (index == -1) {
		lf_debug("Could not find counterpart for '%s'. Attempting to load it.", module->name);
		lf_load(module->data, *module->psize, module->device);
		index = fld_index(module->identifier);
	}
	lf_assert(index != -1, failure, E_MODULE, "No counterpart for the module '%s' was found on the device '%s'. Load the module first.", module->name, module->device->configuration.name);
	module->index = index | FMR_USER_INVOCATION_BIT;
	return lf_success;
failure:
	return lf_error;
}

/* Debugging functions for displaying the contents of various FMR related data structures. */

void lf_debug_call(struct _fmr_invocation *call) {
	printf("call\n");
	printf("\t└─ module:\t\t0x%x\n", call->index);
	printf("\t└─ function:\t0x%x\n", call->function);
	char *typestrs[] = { "int8", "int16", "void", "int32", "int", "", "ptr", "int64" };
	printf("\t└─ return:\t\t%s\n", typestrs[call->ret & 0x7]);
	printf("\t└─ types:\t\t0x%x\n", call->types);
	printf("\t└─ argc:\t\t0x%x (%d arguments)\n", call->argc, call->argc);
	printf("arguments\n");
	/* Calculate the offset into the packet at which the arguments will be loaded. */
	uint8_t *offset = call->parameters;
	lf_types types = call->types;
	for (lf_argc i = 0; i < call->argc; i ++) {
		lf_type type = types & lf_max_t;
		lf_arg arg = 0;
		memcpy(&arg, offset, lf_sizeof(type));
		printf("\t└─ %c%s:\t\t0x%llx\n", ((type & (1 << 3)) ? '\0' : 'u'), typestrs[type & 0x7], arg);
		offset += lf_sizeof(type);
		types >>= 4;
	}
	printf("\n");
}

static int lf_debug_level = LF_DEBUG_LEVEL_OFF;

void lf_set_debug_level(int level) {
	lf_debug_level = level;
}

void lf_debug_packet(struct _fmr_packet *packet, size_t length) {
	if (lf_debug_level != LF_DEBUG_LEVEL_ALL) return;

	if (packet->header.magic == FMR_MAGIC_NUMBER) {
		printf("header\n");
		printf("\t└─ magic:\t\t0x%x\n", packet->header.magic);
		printf("\t└─ checksum:\t0x%x\n", packet->header.checksum);
		printf("\t└─ length:\t\t%d bytes (%.02f%%)\n", packet->header.length, (float) packet->header.length/sizeof(struct _fmr_packet)*100);
		char *classstrs[] = { "standard", "user", "push", "pull", "send", "receive", "load", "event" };
		printf("\t└─ class\t\t%s\n", classstrs[packet->header.class]);
		struct _fmr_invocation_packet *invocation = (struct _fmr_invocation_packet *)(packet);
		struct _fmr_push_pull_packet *pushpull = (struct _fmr_push_pull_packet *)(packet);
		switch (packet->header.class) {
			case fmr_standard_invocation_class:
				lf_debug_call(&invocation->call);
			break;
			case fmr_user_invocation_class:
				lf_debug_call(&invocation->call);
			break;
			case fmr_push_class:
			case fmr_pull_class:
				printf("length:\n");
				printf("\t└─ length:\t\t0x%x\n", pushpull->length);
				lf_debug_call(&pushpull->call);
			break;
			default:
				printf("Invalid packet class.\n");
			break;
		}
		for (size_t i = 1; i <= length; i ++) {
			printf("0x%02x ", ((uint8_t *)packet)[i - 1]);
			if (i % 8 == 0 && i < length - 1) printf("\n");
		}
	} else {
		printf("Invalid magic number (0x%02x).\n", packet->header.magic);
	}
	printf("\n\n-----------\n\n");
}

void lf_debug_result(struct _fmr_result *result) {
	if (lf_debug_level != LF_DEBUG_LEVEL_ALL) return;

	printf("response:\n");
	printf("\t└─ value:\t0x%x\n", result->value);
	printf("\t└─ error:\t0x%hhx\n", result->error);
	printf("\n-----------\n\n");
}
