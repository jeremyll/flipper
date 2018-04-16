#include <flipper.h>

static int lf_debug_level = LF_DEBUG_LEVEL_OFF;

void lf_set_debug_level(int level) {
	lf_debug_level = level;
}

void lf_debug_call(struct _fmr_invocation *call) {
#ifdef __LF_DEBUG__
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
#endif
}

void lf_debug_packet(struct _fmr_packet *packet, size_t length) {
#ifdef __LF_DEBUG__
	if (lf_debug_level != LF_DEBUG_LEVEL_ALL) return;

	if (packet->header.magic == FMR_MAGIC_NUMBER) {
		printf("header\n");
		printf("\t└─ magic:\t\t0x%x\n", packet->header.magic);
		printf("\t└─ checksum:\t0x%x\n", packet->header.checksum);
		printf("\t└─ length:\t\t%d bytes (%.02f%%)\n", packet->header.length, (float) packet->header.length/sizeof(struct _fmr_packet)*100);
		char *classstrs[] = { "exec", "push", "pull", "dyld" };
		printf("\t└─ class\t\t%s\n", classstrs[packet->header.class]);
		struct _fmr_invocation_packet *invocation = (struct _fmr_invocation_packet *)(packet);
		struct _fmr_push_pull_packet *pushpull = (struct _fmr_push_pull_packet *)(packet);
		struct _fmr_dyld_packet *dyld = (struct _fmr_dyld_packet *)(packet);
		switch (packet->header.class) {
			case fmr_execute_class:
				lf_debug_call(&invocation->call);
			break;
			case fmr_push_class:
			case fmr_pull_class:
				printf("length:\n");
				printf("\t└─ length:\t\t0x%x\n", pushpull->length);
				lf_debug_call(&pushpull->call);
			break;
			case fmr_dyld_class:
				printf("module:\n");
				printf("\t└─ module:\t\t'%s'\n", dyld->module);
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
#endif
}

void lf_debug_result(struct _fmr_result *result) {
#ifdef __LF_DEBUG__
	if (lf_debug_level != LF_DEBUG_LEVEL_ALL) return;

	printf("response:\n");
	printf("\t└─ value:\t0x%x\n", result->value);
	printf("\t└─ error:\t0x%hhx\n", result->error);
	printf("\n-----------\n\n");
#endif
}