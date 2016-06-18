int startContinuousTransfer(struct MCCDevice_t *dev,
			unsigned int rate,
			struct dataBuffer_t *buffer,
                        int samps, double delay);
void stopContinuousTransfer(void);

