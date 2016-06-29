#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libusb.h>

#include "mccdevice.h"

#define STRINGMESSAGE 0x80
#define MAX_MESSAGE_LENGTH 64

/* Is the specified product ID is an MCC product ID? */
static int isMCCProduct(int idProduct) {

	switch(idProduct) {
		case USB_2001_TC:
		case USB_7202:
		case USB_7204:
		case USB_1208_FS_PLUS:
		case USB_1608_FS_PLUS:
		case USB_1608_GX:
		case USB_1608_GX_2AO:
			return 1;
		default:
			return 0;
	}
}

static char *toNameString(int idProduct) {

	switch(idProduct) {
		case USB_2001_TC:	return "USB-2001-TC";
		case USB_7202:		return "USB-7202";
		case USB_7204:		return "USB-7204";
		case USB_1608_GX:	return "USB-1608GX";
		case USB_1608_GX_2AO:	return "USB-1608GX-2AO";
		case USB_1608_FS_PLUS:	return "USB-1608-FS-PLUS";
		case USB_1208_FS_PLUS:	return "USB-1608-FS-PLUS";
		default:		return "Invalid Product ID";
	}
}

static void libUSBError(int err) {

	switch(err) {
		case LIBUSB_ERROR_TIMEOUT:
			fprintf(stderr,"USB timeout\n");
			break;
		case LIBUSB_ERROR_PIPE:
			fprintf(stderr,"USB pipe\n");
			break;
		case LIBUSB_ERROR_NO_DEVICE:
			fprintf(stderr,"USB no device\n");
			break;
		default:
			fprintf(stderr,"USB unknown error\n");
			break;
	}
}

/* Find the input endpoint from an endpoint descriptor */
static unsigned char getEndpointInAddress(unsigned char* data, int data_length) {

	int descriptorType;
	int length;
	int index = 0;

	while (1) {

		length = data[index];
		descriptorType = data[index + 1];

		if (length == 0) break;

		if (descriptorType != 0x05) {
			index += length;
		}
		else {
			if ((data[index + 2] & 0x80) != 0) {
				return data[index + 2];
			}
			else {
				index += length;
			}
		}

		if (index >= data_length) break;
	}

	return 0;
}

/* Find the output endpoint from an endpoint descriptor */
static unsigned char getEndpointOutAddress(unsigned char* data, int data_length) {

	int descriptorType;
	int length;
	int index = 0;

	while (1) {

		length = data[index];
		descriptorType = data[index + 1];

		if (length == 0) break;

		if (descriptorType != 0x05) {
			index += length;
		}
		else {
			if ((data[index + 2] & 0x80) == 0) {
				return data[index + 2];
			}
			else {
				index += length;
			}
		}

		if (index >= data_length) break;
	}

	return 0;
}


static unsigned short getBulkPacketSize(unsigned char* data, int data_length) {

	int descriptorType;
	int length;
	int index = 0;

	while(1){
		length = data[index];
		descriptorType = data[index+1];

		if (length == 0) break;

		if (descriptorType != 0x05) {
			index += length;
		} else {
			if ((data[index+2] & 0x80) != 0) {
				/* found the packet size */
				return (unsigned short)(data[index+5] << 8) | (unsigned short)(data[index + 4]);
			} else {
				index += length;
			}
		}
	}
	return 0;
}



/* Get the device input and output endpoints */
static void getScanParams(struct MCCDevice_t *dev) {

	int numBytesTransferred;

	unsigned char epDescriptor[64];

	numBytesTransferred = libusb_control_transfer(dev->dev_handle, STRINGMESSAGE, LIBUSB_REQUEST_GET_DESCRIPTOR,
		(0x02 << 8) | 0, 0, epDescriptor, 64, 1000);

	if (numBytesTransferred < 0) {
		libUSBError(numBytesTransferred);
	}

	dev->endpoint_in = getEndpointInAddress(epDescriptor, numBytesTransferred);
	dev->endpoint_out = getEndpointOutAddress(epDescriptor, numBytesTransferred);
	dev->bulkPacketSize = getBulkPacketSize(epDescriptor, numBytesTransferred);
}

static void initDevice(int id, struct MCCDevice_t *dev) {

	switch(id) {

	case USB_7202:
	case USB_1608_FS_PLUS:
		dev->maxCounts = 0xFFFF;
		break;

	case USB_7204:
	case USB_1208_FS_PLUS:
		dev->maxCounts = 0xFFF;
		break;

	default:
		break;
	}
}



/* Find the first available device where product ID == idProduct */
int MCCDevice_init(int id, struct MCCDevice_t *dev) {

	int i;
	int found = 0;
	ssize_t sizeOfList;
	struct libusb_device_descriptor desc;
	libusb_device* device;

	/* Check if the product ID is a valid MCC product ID */
	if (!isMCCProduct(id)) {
		fprintf(stderr,"Invalid MCC product ID!\n");
		return -1;
	}

	/* Initialize USB library */
	if (libusb_init(NULL) != 0) {
		fprintf(stderr,"Cannot enable USB library!\n");
		return -1;
	}

	/* Get the list of USB devices connected to the PC */
	sizeOfList = libusb_get_device_list(NULL, &(dev->list));

	/* Traverse the list of USB devices to find the requested device */
	for(i=0; (i<sizeOfList) && (!found); i++) {
		device = dev->list[i];
		libusb_get_device_descriptor(device, &desc);
		if (desc.idVendor == MCC_VENDOR_ID &&
			desc.idProduct == id) {
			fprintf(stderr,"Found %s\n",toNameString(id));

			/* Open the device */
			if (!libusb_open(device, &dev->dev_handle)) {

				/* Claim interface with the device */
				if (!libusb_claim_interface(dev->dev_handle,0)) {
					/* Get input and output endpoints */
					getScanParams(dev);
					found = 1;
				}
			}
			else {
				fprintf(stderr,"Error opening USB\n");
				return -1;
			}
		}
	}

	if (!found) {
		fprintf(stderr,"Error finding device\n");
		return -1;
	}
	else {
		dev->idProduct = id;
		initDevice(id,dev);
	}
	return 0;
}

/* Free memory and devices */
void MCCDevice_free(struct MCCDevice_t *dev) {
	libusb_release_interface(dev->dev_handle, 0);
	libusb_close(dev->dev_handle);
	libusb_free_device_list(dev->list, 1);
	libusb_exit(NULL);
}

/* Send a message to the device */
static int sendControlTransfer(struct MCCDevice_t *dev, char *message) {

	int numBytesTransferred;
	uint16_t length;
	unsigned char data[MAX_MESSAGE_LENGTH];
	int i;

	fprintf(stderr,"Sending: %s /",message);

	length = strlen(message);
	for (i = 0; i < MAX_MESSAGE_LENGTH; i++) {
		data[i] = (i < length) ? message[i] : 0;
	}

	numBytesTransferred = libusb_control_transfer(dev->dev_handle,
			LIBUSB_REQUEST_TYPE_VENDOR + LIBUSB_ENDPOINT_OUT,
			STRINGMESSAGE, 0, 0, data, MAX_MESSAGE_LENGTH, 1000);

	if(numBytesTransferred < 0) {
		fprintf(stderr,"Error USB transfer\n");
		return -1;
	}
	return 0;
}

/* Receive a message from the device. */
/* This should follow a call to sendControlTransfer. */
/* It will return a pointer to at most a 64 character array. */
static char *getControlTransfer(struct MCCDevice_t *dev, char *message) {

	int messageLength;

	messageLength = libusb_control_transfer(dev->dev_handle,
			LIBUSB_REQUEST_TYPE_VENDOR + LIBUSB_ENDPOINT_IN,
			STRINGMESSAGE, 0, 0, (unsigned char *)message, 64, 1000);

	if (messageLength < 0) {
		fprintf(stderr,"Error receiving\n");
		return NULL;
	}
	fprintf(stderr,"Got: %s\n",message);

	return message;
}


/* Will return at most a 64 character array. */
/* Returns response if transfer successful, null if not */
char *sendMessage(struct MCCDevice_t *dev,
		char *in_message, char *out_message) {

	int result;

	result=sendControlTransfer(dev,in_message);
	if (result<0) {
		fprintf(stderr,"Error!\n");
		return NULL;
	}
	else {
	        return getControlTransfer(dev,out_message);
	}
}

void flushInputData(struct MCCDevice_t *dev) {

	int bytesTransfered = 0;
	int status;
        unsigned char buf[dev->bulkPacketSize];

	do {
		status = libusb_bulk_transfer(dev->dev_handle,
			dev->endpoint_in, buf, dev->bulkPacketSize,
			&bytesTransfered, 200);
	} while (bytesTransfered > 0 && status == 0);
}

