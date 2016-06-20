#include <stdio.h>
#include <pthread.h>
#include <libusb.h>
#include <errno.h>
#include <unistd.h>

#include "databuffer.h"
#include "mccdevice.h"
#include "poll_thread.h"

int thread_alive=1;
pthread_t our_thread;

struct usb_stuff_t {
	struct dataBuffer_t *buffer;
	libusb_device_handle* dev_handle;
	int endpoint_in;
	int numSamples;
	int delay;
};

static struct usb_stuff_t usb;


static void *read_usb(void *arg) {

	int err = 0, transferred = 0;
	struct usb_stuff_t *args;

	args=(struct usb_stuff_t *)arg;

	unsigned char *dataAsByte = (unsigned char*)(args->buffer->data);
	unsigned int timeout = 2000000;///(64*argPtr->rate);

	args->buffer->currIndex = 0;

	while (thread_alive) {
		usleep(args->delay);
		err =  libusb_bulk_transfer(
					args->dev_handle,
					args->endpoint_in,
					&dataAsByte[args->buffer->currIndex*2],
					args->numSamples,
					&transferred,
					timeout);
	        args->buffer->currIndex += (transferred/2);

		/* a timeout may indicate that some data was transferred, but not all */
		if (err == LIBUSB_ERROR_TIMEOUT && transferred > 0) {
			err = 0;
		}

	        if (err < 0) {
			fprintf(stderr,"USB transfer ERROR!\n");
		}

		if (args->buffer->currIndex >= (args->buffer->numPoints)) {
			args->buffer->currIndex = 0;
		}
	}

	return NULL;
}


int startContinuousTransfer(struct MCCDevice_t *dev,
				struct dataBuffer_t *buffer,
                        	int samps, double delay) {

	int code=0;

	usb.buffer=buffer;
	usb.dev_handle=dev->dev_handle;
	usb.endpoint_in=dev->endpoint_in;
	usb.numSamples=samps;
	usb.delay=delay;

	code = pthread_create(&our_thread, 0, read_usb, &usb);

	return code;
}


void stopContinuousTransfer(void) {

	thread_alive = 0;

	pthread_join(our_thread,NULL);
}
