/* Vendor ID */
#define MCC_VENDOR_ID 0x09db

/* Device IDs */
#define USB_2001_TC 0x00F9
#define USB_7202 0x00F2
#define USB_7204 0x00F0
#define USB_1608_GX 0x0111
#define USB_1608_GX_2AO 0x0112
#define USB_1608_FS_PLUS 0x00EA
#define USB_1208_FS_PLUS 0x00E8

#define SLOPE 0
#define OFFSET 1

#define FIRSTHALF	1
#define SECONDHALF	0

//Error Codes
enum mcc_err{
	MCC_ERR_NO_DEVICE,
	MCC_ERR_INVALID_ID,
	MCC_ERR_USB_INIT,
	MCC_ERR_PIPE,
	MCC_ERR_LIBUSB_TIMEOUT,
	MCC_ERR_TRANSFER_FAILED,
	MCC_ERR_LIBUSB_TRANSFER_STALL,
	MCC_ERR_LIBUSB_TRANSFER_OVERFLOW,
	MCC_ERR_UNKNOWN_LIB_USB_ERR,
	MCC_ERR_INVALID_BUFFER_SIZE,
	MCC_ERR_CANT_OPEN_FPGA_FILE,
	MCC_ERR_FPGA_UPLOAD_FAILED,
	MCC_ERR_ACCESS,
};


struct MCCDevice_t {
	libusb_device **list;
	libusb_device_handle *dev_handle;
	unsigned char endpoint_in;
	unsigned char endpoint_out;
	unsigned short bulkPacketSize;
	int idProduct;
	unsigned short maxCounts;
};

int MCCDevice_init(int idProduct, struct MCCDevice_t *device);
void flushInputData(struct MCCDevice_t *dev);
char *sendMessage(struct MCCDevice_t *dev, char *in_message, char *out_message);
void MCCDevice_free(struct MCCDevice_t *dev);

