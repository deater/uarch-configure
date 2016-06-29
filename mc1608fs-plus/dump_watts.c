#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

int main(int argc, char **argv) {

	int i,result,done=0;
	int fd,temp_int,rate,channels;
	float points[4];
	int ticks=0;
	double watts;
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

			if (isinf(points[i])) {
				done=1;
				break;
			}

			if (result<sizeof(float)) {
				done=1;
				break;
			}

//			printf("%lf\t",points[i]);

		}

		if (done) break;

		/* P=IV */
		/* V=points[1] */
		/* I=V/R = (V/AMP)/R = V/300/.0033 */

		watts=((points[0]/300.0)/0.00333)*points[1];

		printf("%lf\t%lf\t(* %lf %lf *)\n",
			(double)ticks/(double)rate,
			watts,
			points[0],points[1]);
		ticks++;
	}

	read(fd,&temp64,8);
	printf("(* Stop Time %lld.",temp64);
	read(fd,&temp64,8);
	printf("%06lld)\n",temp64);



	return 0;
}
