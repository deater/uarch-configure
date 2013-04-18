#include <stdio.h>

int convert_time_to_seconds(char *time_string) {
   
   int hours=0;
   int minutes=0;
   int seconds=0;
   int total_seconds=0;
   char *pointer;
   
   pointer=time_string;
   
   if (*pointer!='[') return -1;
   pointer++;
   
   hours+=(*pointer)-'0';
   pointer++;
   hours*=10;
   hours+=(*pointer)-'0';
   pointer++;
   
   if (*pointer!=':') {
      fprintf(stderr,"Something wrong with hours: %c\n",*pointer);
      return -1;
   }
   pointer++;   
   
   minutes+=(*pointer)-'0';
   pointer++;
   minutes*=10;
   minutes+=(*pointer)-'0';
   pointer++;

   if (*pointer!=':') {
      fprintf(stderr,"Something wrong with minutes: %c\n",*pointer);
      return -1;
   }
   pointer++;
   
   seconds+=(*pointer)-'0';
   pointer++;
   seconds*=10;
   seconds+=(*pointer)-'0';
   pointer++;

   if (*pointer!=']') {
      fprintf(stderr,"Something wrong with minutes: %c\n",*pointer);
      return -1;
   }

   
   total_seconds=(hours*60*60)+(minutes*60)+seconds;
   
   return total_seconds;
}

void jgraph_header(int maxx,int maxy) {
  printf("newgraph\n");
  printf("X 9\n");
  printf("Y 3\n");
  printf("\n");
  printf("title fontsize 16 y %lf : 5000x5000 DGEMMM, ",(double)maxy);
  printf("IvyBridge Running at 1.2GHz\n");
  printf("\n");
  printf("yaxis size 2 min 0 max %d\n",maxy);
  printf("label font Helvetica fontsize 14 : Power (Watts)\n");
  printf("hash_labels font Helvetica fontsize 14\n");
  printf("\n");
  printf("xaxis size 8 min 0 max %d\n",maxx);
  printf("label font Helvetica fontsize 14 : Time (s)\n");
  printf("hash_labels font Helvetica fontsize 14\n");
  printf("\n");
  printf("newcurve\n");
  printf("linetype solid color 0.0 0.0 1.0\n");
  printf("pts\n");
}

int main(int argc, char **argv) {
   
   char input[BUFSIZ];
   char *result;
   char time_string[BUFSIZ];
   double watts;
   int start_seconds=0;
   int seconds=0;
   int total_seconds;
   int last_seconds=0;

   double average_watts=0.0;
   double total_energy=0.0;

   jgraph_header(490,8); 
  
   while(1) {
	result=fgets(input,BUFSIZ,stdin);
	if (result==NULL) break;
        sscanf(input,"%s %lf",time_string,&watts);
        seconds=convert_time_to_seconds(time_string);
        if (start_seconds==0) start_seconds=seconds;      
        printf("%d %lf\n",seconds-start_seconds,watts);
        if (last_seconds!=0) total_energy+=watts*(double)(seconds-last_seconds);
	last_seconds=seconds;
   }
   total_seconds=seconds-start_seconds;

   average_watts=total_energy/(double)total_seconds;

   printf("(* Total time = %ds      *)\n",total_seconds);
   printf("(* Average Watts = %.3fW *)\n",average_watts);
   printf("(* Total energy = %.3fJ  *)\n",total_energy);
   
   return 0;
   
}
