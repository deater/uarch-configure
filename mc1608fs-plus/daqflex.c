/* Code to read from the Measurement Computing USB_1208_FS_PLUS */

/* Based on code from: https://github.com/kienjakenobi/daqflex	*/
/* See the DAQFlex Message-based Firmware Specification */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <libusb.h>

#include "databuffer.h"
#include "mccdevice.h"
#include "poll_thread.h"

/* Values used for calibration */
float calSlope[16];
float calOffset[16];
int minVoltage;
int maxVoltage;


/* Cal constants are only valid for the current set range.	*/
/* If the range is changed, the cal constants will need		*/
/* to be updated by running this function again.		*/

void fillCalConstants(struct MCCDevice_t *dev,
	unsigned int lowChan, unsigned int highChan) {

	unsigned int currentChan;
	char slope_string[BUFSIZ],offset_string[BUFSIZ];
	char response[64];

	for(currentChan=lowChan; currentChan <= highChan; currentChan++){

        	/* Set up messages */
		sprintf(slope_string,"?AI{%d}:SLOPE",currentChan);
		sprintf(offset_string,"?AI{%d}:OFFSET",currentChan);

		/* Send the message and put the values into an	*/
		/* array containing all channel cal data	*/
		sendMessage(dev,slope_string,response);
		calSlope[currentChan] = atof(response+12);

		sendMessage(dev,offset_string,response);
		calOffset[currentChan] = atof(response+13);

		fprintf(stderr,"Channel %d Calibration Slope: %lf Offset: %lf\n",
			currentChan,calSlope[currentChan],
			calOffset[currentChan]);
    }
}

/* scale and calibrate data */
float scaleAndCalibrateData(unsigned short data,
		float minVoltage, float maxVoltage,
		float scale, float offset,
		int maxCounts) {

	float calibratedData;
	float scaledAndCalibratedData;
	float fullScale = maxVoltage - minVoltage;

	/* Calibrate the data */
	calibratedData = (float)data*scale + offset;

	/* Scale the data */
	scaledAndCalibratedData = (calibratedData/(float)maxCounts)*fullScale
			+ minVoltage;

	return scaledAndCalibratedData;
}


void displayAndWriteData(unsigned short* data, int transferred,
			int num_channels, int maxCounts, FILE* output,
			int binary_output) {

	int currentChan, currentData=0;
	float fixedData;

	while (currentData < transferred) {
		for(currentChan=0; currentChan < num_channels; currentChan++) {
			/* Scale the data */
			fixedData = scaleAndCalibrateData(data[currentData],
					minVoltage, maxVoltage,
					calSlope[currentChan],
					calOffset[currentChan],
					maxCounts);
			if (binary_output) {
				fwrite(&fixedData,sizeof(float),1,output);
			}
			else {
				/* Output all data to a csv file */
				fprintf(output,"%lf,",fixedData);
			}

			currentData++;
		}
		if (!binary_output) fprintf(output,"\n");
	}
}

static void print_help(char *exe_name, int version_only) {

	printf("Daqflex version 0.1\n\n");
	if (!version_only) {
		printf("Usage:\t%s -h -v\n",exe_name);
		printf("\t-b\t: generate binary output file\n");
		printf("\t-h\t: this help message\n");
		printf("\t-o name\t: output filename (- for stdout)\n");
		printf("\t-r rate\t: rate to sample\n");
		printf("\t-v\t: version info\n");
	}
	exit(0);
}

int done=0;

static void ctrlc_handler(int sig, siginfo_t *foo, void *bar) {

        done=1;
}


int main(int argc, char **argv) {

	unsigned int lowChan = 0;
	unsigned int highChan = 3;
	unsigned int num_channels = highChan-lowChan+1;
	int rate = 2048;
	int counter = 0;
	FILE *output;
	struct MCCDevice_t device;
	int result;
	char out_message[64];
	int device_type;

	int sample_times;
	/* Half of the buffer will be handled at a time. */
	int lastHalfRead = SECONDHALF;
	unsigned int delay;
	struct dataBuffer_t buffer;
	char temp_message[BUFSIZ];
	char filename[BUFSIZ];
	int c,buffer_size;
	struct sigaction sa;
	int binary_output=0;

	/* hardcoded, should set from a command line arg */
	device_type=USB_1208_FS_PLUS;
	strcpy(filename,"testfile.csv");

	/* Check command-line args */
	opterr=0;
	while ((c = getopt(argc,argv,"bhr:o:v")) != -1) {
		switch(c) {
			case 'b':
				binary_output=1;
				break;
			case 'h':
				print_help(argv[0],0);
				break;
			case 'o':
				strcpy(filename,optarg);
				break;
			case 'r':
				rate=atoi(optarg);
				break;
			case 'v':
				print_help(argv[0],1);
				break;
			default:
				fprintf(stderr,"Unknown option %c\n",c);
			return -1;
		}

	}

	/* For continuous scan				*/
	/* Minimum USB transfer for this device is 64B	*/
	/* Samples are 16-bits (2 Bytes)		*/
	/* So sample buffer must be multiple of 64B	*/

	/* Allocate space for 1s of samples at least */
	buffer_size=(rate)*(num_channels)*2;
	if (buffer_size<64) {
		buffer_size=64;
	}
	/* Round to a multiple of 64 */
	buffer_size=((buffer_size+63)/64)*64;

	sample_times=(buffer_size/num_channels)/2;
	delay = (sample_times*100000)/(num_channels*rate*2);

	printf("Using samples: %d delay %dus\n",sample_times,delay);

	if (!strcmp(filename,"-")) {
		output=stdout;
	}
	else {
		output=fopen(filename,"w");
		if (output==NULL) {
			fprintf(stderr,"Could not open file %s\n",filename);
			return -1;
		}
	}

	/* setup control-c handler */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = ctrlc_handler;
	sa.sa_flags = SA_SIGINFO;

	if (sigaction( SIGINT, &sa, NULL) < 0) {
		fprintf(stderr,"Error setting up signal handler\n");
		return -1;
	}


        /*Initialize the device */
	/* note that MCCDevice(int idProduct, string mfgSerialNumber) */
	/* could be used to find a device given a unique manufacturer */
	/* serial number. */

	result=MCCDevice_init(device_type,&device);
	if (result<0) {
		fprintf(stderr,"Error initializing!\n");
		return -1;
	}

	/* create a buffer for the scan data */
	buffer.data=calloc(buffer_size,1);
	if (buffer.data==NULL) {
		fprintf(stderr,"Error allocating buffer\n");
		return -1;
	}

	buffer.numPoints = (sample_times*num_channels);
	buffer.currIndex=0;



	/* Flush out any old data from the buffer */
	flushInputData(&device);

	/* Stop scanning, in case we crashed or otherwise	*/
	/* Were still scanning from a previous run		*/
	sendMessage(&device,"AISCAN:STOP",out_message);


	/* Query the device a bit */
        sendMessage(&device,"?AI:RES",out_message);


	/***************************/
	/* Configure an input scan */
	/***************************/

	/* AISCAN:XFRMODE=*/
	/* BLOCKIO */
	/* SINGLEIO */
	/* BURSTIO */
        sendMessage(&device,"AISCAN:XFRMODE=BLOCKIO",out_message);


	/* Set absolute or differential modde */
	/* AI:CHMODE= DIFF or SE. (differential or single-ended)*/
	//sendMessage(device,"AI:CHMODE=DIFF");

	/* Set the voltage range on the device */
	/* AISCAN:RANGE= */
	/* On the 1208-plus the options are */
	/* For differential: */
	/*    BIP20V, BIP10V, BIP5V, BIP4V, BIP2PT5V, BIP1PT25V, BIP1V */
	/* For single-ended: */
	/*    BIP10V */
	/* NOTE!  When differential ADV only has 11-bit resolution */
	/*        rather than 12-bit with single-ended */
	/*        See USB-120FS-Plus user guide page 11 */
	/*        But table on page 20 says the opposite? */
	sendMessage(&device,"AISCAN:RANGE=BIP5V",out_message);
	/* Set range for scaling purposes */
        minVoltage = -5;
        maxVoltage = 5;

	/* Set channels, 0-7 for single-ended, 0-3 for differential */
	sprintf(temp_message,"AISCAN:LOWCHAN=%d",lowChan);
	sendMessage(&device,temp_message,out_message);
	sprintf(temp_message,"AISCAN:HIGHCHAN=%d",highChan);
	sendMessage(&device,temp_message,out_message);

	sprintf(temp_message,"AISCAN:RATE=%d",rate);
	sendMessage(&device,temp_message,out_message);

	sprintf(temp_message,"?AISCAN:RATE");
	sendMessage(&device,temp_message,out_message);



	/* SAMPLES=0 means scan continuously */
	sendMessage(&device,"AISCAN:SAMPLES=0",out_message);

        /* Fill cal constants for later use */
        fillCalConstants(&device,lowChan, highChan);

	/* Start the scan on the device */
	sendMessage(&device,"AISCAN:START",out_message);

	/* Start collecting data in the background */
	/* Data buffer info will be stored in the buffer object */
	startContinuousTransfer(&device, &buffer,
		buffer.numPoints/2, delay);

	printf("Start time %ld\n",time(NULL));
	if (binary_output) {
		int temp_value=0;

		/* version */
		fwrite(&temp_value,sizeof(int),1,output);

		/* time */
		temp_value=time(NULL);
		fwrite(&temp_value,sizeof(int),1,output);

		/* rate */
		temp_value=rate;
		fwrite(&temp_value,sizeof(int),1,output);
	}

	printf("Press ^C to exit\n");


        /* Main loop of program */
	while(!done) {
		usleep(delay);

		/* Data is placed into a circular buffer	*/
		/* Make sure to check buffer often enough so    */
		/* that you do not lose data			*/
		/* Only half the buffer is read at a time to avoid collisions */
		if ((buffer.currIndex > buffer.numPoints/2) &&
			(lastHalfRead == SECONDHALF)) {
			// printf("First Half Ready\n");
			displayAndWriteData(buffer.data,
						buffer.numPoints/2,
						num_channels,
						device.maxCounts,
						output,
						binary_output);
			lastHalfRead = FIRSTHALF;
			counter++;
		}
		else if ((buffer.currIndex < buffer.numPoints/2) &&
			(lastHalfRead == FIRSTHALF)) {
			//printf("Second Half Ready\n");
			displayAndWriteData(&buffer.data[buffer.numPoints/2],
						buffer.numPoints/2,
						num_channels,
						device.maxCounts,
						output,
						binary_output);
			lastHalfRead = SECONDHALF;
			counter++;
		}
		/* blink LED if every 10 buffer reads */
		/*if (counter%10==0) sendMessage(device,"DEV:FLASHLED/1");*/
	}

	fprintf(stderr,"Done\n");
	printf("End time %ld\n",time(NULL));

	stopContinuousTransfer();
	sendMessage(&device,"AISCAN:STOP",out_message);

	/* check status for debugging purposes */
	sendMessage(&device,"?AISCAN:STATUS",out_message);

	/* close the output file */
	if (output!=stdout) fclose(output);

	/* Cleanup */
	MCCDevice_free(&device);

	return 0;
}
