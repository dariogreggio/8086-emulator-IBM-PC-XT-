#include <stdio.h>
#include <io.h>

int d[256];
int debug,verbose=1;

int aspetta() {
  int i;

  i=10000;
  while((inp(0x1F7) & 128) && --i);
  if(i)
    return 1;
  else
    return 0;
  }

int testHD(int i) {

  outp(0x1F6, 0xA0 | (i << 4));
  aspetta();
  outp(0x1F6, 0xA0 | (i << 4));
  //PRINT INP(0x1F6)
    outp(0x1F4, 0xAA);
  //PRINT INP(0x1F4)
    outp(0x1F4, 0x55);
  //PRINT INP(0x1F4)
  if(inp(0x1F7) & 0x40) {		// anche 0x10, parrebbe
    printf("Disco %d presente\n",i);
    return 1;
    }
  else {
    printf("Disco %d non rilevato\n",i);
    return 0;
    }
  }

int leggiHD(int i) {
  int j;
  
  outp(0x1F6,0xA0 | (i << 4));
  aspetta();
  outp(0x3F6, 0);
  for(j= 0x1F1; j<=0x1F5; j++)
    outp(j, 0);
  outp(0x1F6, 0xA0 | (i << 4));
  if(inp(0x1F7) != 0xff && inp(0x1F7) & 0x40) {     // non floating; serve anche 0x10, parrebbe
    printf("Disco %d:\n", i);
    outp(0x1F7, 0xEC);
    aspetta();
    j=10000;
    while(!(inp(0x1F7) & 8) && --j);
    if(!j)
      return 0;
    for(j = 0; j<256; j++)
      d[j] = inpw(0x1F0);
    if(verbose) {
        printf("\tNumero di serie: ");
        for(j=10; j<=19; j++) {
          putchar(*((char *)(&d[j])+1));
          putchar((char)d[j]);
          }
        putchar('\n');
        printf("\tModello:         ");
        for(j=27; j<=46; j++) {
          putchar(*((char *)(&d[j])+1));
          putchar((char)d[j]);
          }
        putchar('\n');
        printf("\tFirmware rev.:   ");
        for(j=23; j<=26; j++) {
          putchar(*((char *)(&d[j])+1));
          putchar((char)d[j]);
          }
        putchar('\n');
        printf("\n\tConfigurazione:                    %04X\n",d[0]);
        printf("\tTransfer rate:                     %s\n",d[0] & 0x100 ? "<=5Mbps" : 
					(d[0] & 0x200 ? "tra 5 e 10Mbps" : d[0] & 0x400 ? ">10Mbps" : "nd")
					);
        printf("\tTipo:                              %s\n",d[0] & 0x80 ? "rimovibile" : 
					(d[0] & 0x40 ? "fisso" : "nd")
					);
        if(d[0] & 0x8000)
	      printf("\tUnità nastro\n");
        printf("\t                                   %s\n",d[0] & 0x04 ? "soft sectored" : 
					(d[0] & 0x02 ? "hard sectored" : "nd")
					);
					// https://www.os2museum.com/wp/whence-identify-drive/
/*							General configuration bit-significant information:
15 0 reserved for non-magnetic drives
14 1=format speed tolerance gap required
13 1=track offset option available
12 1=data strobe offset option available
11 1=rotational speed tolerance is > 0,5%
10 1=disk transfer rate > 10 Mbs
 9 1=disk transfer rate > 5Mbs but <= 10Mbs
 8 1=disk transfer rate <= 5Mbs
 7 1=removable cartridge drive
 6 1=fixed drive
 5 1=spindle motor control option implemented
 4 1=head switch time > 15 usec
 3 1=not MFM encoded
 2 1=soft sectored
 1 1=hard sectored
 0 0=reserved*/

        printf("\n\tCilindri:                          %u,%u\n",d[1],d[2]);
        printf("\tTestine:                           %u\n",d[3]);
        printf("\tSettori:                           %u\n",d[6]);
        printf("\tByte per traccia,settore:          %u,%u\n",d[4],d[5]);
        printf("\tDimensione buffer controller:      %uKB\n",((long)d[21]) /2);  // * 512 e / 1024
        printf("\tByte ECC:                          %u\n",d[22]);
        printf("\tSettori/interrupt per RW multiple: %u\n",d[47]);
        if(d[48])
	      printf("\tdouble-word transfer possibili\n");
	    if(d[60] || d[61]) {
	      unsigned long s28;
	      s28=d[60] | ((unsigned long )d[61] << 16);
	      printf("\tSettori LBA:                       %lu\n",s28);
//			sectorData[120]=LOBYTE(LOWORD(0));	sectorData[121]=HIBYTE(LOWORD(0));
			// settori in modo LBA28
		  }
        if(d[83] & 0x400) {
//			sectorData[166]=B8(00000000);	sectorData[167]=B8(00000000);
			// B10=1 se LBA48
	      printf("\tLBA48\n");
	      }
        if(d[88]) {
//		sectorData[176]=B8(00000000);	sectorData[177]=B8(00000000);
			// DMA mode: lo-byte quelli supportati e hi-							
	      printf("\tSupporto DMA/UDMA:                ");
	      for(i=0; i<4; i++) {
		    if(d[88] & (1<<i)) {
		      printf(" mode %u (%s);",d[88] & (1<<i),d[88] & (1<<(i+8)) ? "in uso" : "non in uso");
		      }
		    }
	      printf("\n");
	      }
        if(d[93] & 0x800) {
//			sectorData[186]=B8(00000000);	sectorData[187]=B8(00000000);
			// se Master drive, B11 indica cavo 80 pin presente
	      printf("\tCavo 80 fili rilevato\n");
	      }
	    if(d[100] || d[101] || d[102] || d[103]) {
	      unsigned long s48l,s48h;
	      printf("\tSettori LBA48: %lu\n",s48l);		// FINIRE!!
//		sectorData[200]=LOBYTE(LOWORD(0));	sectorData[201]=HIBYTE(LOWORD(0));
				// settori in modo LBA48
		  }

        printf("\n\tCapacit… del disco:                %luMB (%lu UF)\n",(((long)d[6])*d[3]*d[1]*512l)/1048576l,(((long)d[5])*d[6]*d[3]*d[1])/1048576l);
        }
    else {
      printf("%d,%d,%d\n",d[1],d[3],d[6]);
      }
    return 1;
    }
  else {
    return 0;
    }
  }

int main(int argc, char **argv) {
  FILE *f;
  int i;
        
  if(argc>1 /* alla veloce!!*/)
  	debug=1;
  fputs("Hard Disk Info 1.10 - (C) ADPM Synthesis 1994-2025\n",stderr);
  fputc('\n',stderr);
  i=*(char far *)0x00400075l;
  printf("Il Bios riconosce %u hard disk\n",i);
  i = testHD(0);
  i = testHD(1);
  putchar('\n');
  i = leggiHD(0);
  putchar('\n');
  i = leggiHD(1);

  if(debug) {
    f=fopen("hdtest.dat","wb");
    if(f) {
      fwrite(&d,256,2,f);
      fclose(f);
      }
    }
  }


