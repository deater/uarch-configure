#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

int main(int argc, char **argv) {

	int i,result,done=0;
	int fd,temp_int,rate,channels;
	float points[4];
	int ticks=0;
	long long temp64;
	float temp_float;

	float last[2]={-5.0, -5.0};
	float prev[4];

	int threshold=1;

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
	write(1,&temp_int,4);

	fprintf(stderr,"(* Version %d *)\n",temp_int);

	read(fd,&temp64,8);
	write(1,&temp64,8);
	fprintf(stderr,"(* Start Time %lld.",temp64);
	read(fd,&temp64,8);
	write(1,&temp64,8);
	fprintf(stderr,"%06lld)\n",temp64);

	read(fd,&temp_int,4);
	write(1,&temp_int,4);
	rate=temp_int;
	fprintf(stderr,"(* Rate %d Hz *)\n",rate);

	threshold=rate/500;
	if (threshold<1) threshold=1;

	read(fd,&temp_int,4);
	write(1,&temp_int,4);
	channels=temp_int;
	fprintf(stderr,"(* Channels %d *)\n",channels);

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
		//		printf("%lf:\t",(double)ticks/(double)rate);
			}

		//	printf("%lf\t",points[i]);

		}
		//printf("\n");

		if (done) break;

		if  ((last[0]<0.0) && (last[1]>0.0) && (points[3]<0.0)) {
			fprintf(stderr,"Glitch1!\n");
			prev[3]=-prev[3];
		}
		else if ((last[0]>0.0) && (last[1]<0.0) && (points[3]>0.0)) {
			fprintf(stderr,"Glitch2!\n");
			prev[3]=-prev[3];
		}

		if (ticks!=0) write(1,prev,4*sizeof(float));

		for(i=0;i<4;i++) {
			prev[i]=points[i];
		}

		last[0]=last[1];
		last[1]=points[3];

		ticks++;
	}

	write(1,prev,4*sizeof(float));

	temp_float=INFINITY;
	write(1,&temp_float,sizeof(float));

	read(fd,&temp64,8);
	write(1,&temp64,8);
	fprintf(stderr,"(* Stop Time %lld.",temp64);
	read(fd,&temp64,8);
	write(1,&temp64,8);
	fprintf(stderr,"%06lld)\n",temp64);

	return 0;
}
