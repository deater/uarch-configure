all:	mips_config_brpred

mips_config_brpred:	   mips_config_brpred.o
		   ld -o mips_config_brpred mips_config_brpred.o
		   
mips_config_brpred.o:	   mips_config_brpred.s
		   as -o mips_config_brpred.o mips_config_brpred.s
		   
clean:		   
		   rm -f *~ *.o mips_config_brpred
		   
