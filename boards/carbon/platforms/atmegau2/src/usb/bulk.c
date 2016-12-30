#define __private_include__
#include <megausb.h>

/* Receive a packet using the appropriate bulk endpoint. */
int8_t megausb_bulk_receive(uint8_t *destination, lf_size_t length) {

	/* If USB is not configured, return with error. */
	if (!megausb_configured) {
		return -1;
	}

	/* Calculate the timeout value using the frame counter. */
	uint8_t timeout = UDFNUML + DEFAULT_TIMEOUT;

	/* Select the endpoint that has been configured to receive bulk data. */
	UENUM = BULK_OUT_ENDPOINT;

	int total = lf_ceiling(length, BULK_OUT_SIZE);
	for (int i = 0; i < total; i ++) {
		/* Wait until the receiver is ready. */
		while (!(UEINTX & (1 << RWAL))) {
			/* If USB has been detached while in this loop, return with error. */
			if (!megausb_configured) {
				return -1;
			}
			/* If a timeout has occured, return 0 bytes sent. */
			else if (UDFNUML == timeout) {
				return 0;
			}
		}

		/* Transfer the buffered data to the destination. */
		uint8_t len = BULK_OUT_SIZE;
		while (len --) {
			if (length --) {
				/* If there is still valid data to send, load it from the receive buffer. */
				*destination ++ = UEDATX;
			} else {
				/* Otherwise, flush the buffer. */
				break;
			}
		}

		/* Flush the receive buffer and reset the interrupt state machine. */
		UEINTX = (1 << NAKINI) | (1 << RWAL) | (1 << RXSTPI) | (1 << STALLEDI) | (1 << TXINI);
	}

	return BULK_OUT_SIZE;
}

/* Receive a packet using the appropriate bulk endpoint. */
int8_t megausb_bulk_transmit(uint8_t *source, lf_size_t length) {

	/* If USB is not configured, return with error. */
	if (!megausb_configured) {
		return -1;
	}

	/* Calculate the timeout value using the frame counter. */
	uint8_t timeout = UDFNUML + DEFAULT_TIMEOUT;

	/* Select the endpoint that has been configured to receive bulk data. */
	UENUM = BULK_IN_ENDPOINT & ~USB_IN_MASK;

	int total = lf_ceiling(length, BULK_OUT_SIZE);
	for (int i = 0; i < total; i ++) {
		/* Wait until the transmitter is ready. */
		while (!(UEINTX & (1 << RWAL))) {
			/* If USB has been detached while in this loop, return with error. */
			if (!megausb_configured) {
				return -1;
			}
			/* If a timeout has occured, return 0 bytes sent. */
			else if (UDFNUML == timeout) {
				return 0;
			}
		}

		/* Transfer the buffered data to the destination. */
		uint8_t len = BULK_IN_SIZE;
		while (len --) {
			if (length --) {
				/* If there is still valid data to send, load it into the transmit buffer. */
				UEDATX = *source ++;
			} else {
				/* Otherwise, flush the buffer. */
				break;
			}
		}

		/* Flush the transmit buffer and reset the interrupt state machine. */
		UEINTX = (1 << RWAL) | (1 << NAKOUTI) | (1 << RXSTPI) | (1 << STALLEDI);
	}

	return BULK_IN_SIZE;
}