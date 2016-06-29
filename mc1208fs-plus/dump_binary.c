#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

int main(int argc, char **argv) {

	int i,result,done=0;
	int fd,temp_int,rate,channels;
	float points[8];
	int ticks=0;
	long long temp64;

	if (argc>1) {

		fd=open(argv[1],O_RDONLY);
		if (fd<1) {
			fprintf(stderr,"Could not open %s\n",argv[1]);
			return -1;
		}
	}
	else {
		/* stdin */
		fd=0;
	}

	read(fd,&temp_int,4);
	printf("(* Version %d *)\n",temp_int);

	read(fd,&temp64,8);
	printf("(* Start Time %lld.",temp64);
	read(fd,&temp64,8);
	printf("%06lld)\n",temp64);

	read(fd,&temp_int,4);
	rate=temp_int;
	printf("(* Rate %d Hz *)\n",rate);

	read(fd,&temp_int,4);
	channels=temp_int;
	printf("(* Channels %d *)\n",channels);

	while(!done) {

		for(i=0;i<channels;i++) {

			result=read(fd,&points[i],sizeof(float));

			if (result<sizeof(float)) {
				done=1;
				break;
			}

			if (isinf(points[i])) {
				done=1;
				break;
			}

			if (i==0) {
				printf("%lf:\t",(double)ticks/(double)rate);
			}

			printf("%lf\t",points[i]);

		}
		printf("\n");

		ticks++;
	}

	read(fd,&temp64,8);
	printf("(* Stop Time %lld.",temp64);
	read(fd,&temp64,8);
	printf("%06lld)\n",temp64);

	return 0;
}
