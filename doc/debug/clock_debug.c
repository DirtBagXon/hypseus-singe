
/* cc -o clock_debug clock_debug.c -Wno-format */

#include <stdio.h>
#include <time.h>
#include <unistd.h>


#define    DMULTI   10
#define    OMULTI    8
#define    SMULTI    6
#define    FMULTI    4
#define    TMULTI    2

int main(int argc, char **argv)
{
    clock_t start = clock();
    sleep(1);
    clock_t end = clock();

    switch (sizeof(void*)){
     case 4: printf("\nSystem: 32bit\n\n");
      break;
     case 8: printf("\nSystem: 64bit\n\n");
      break;
    }

    printf("sizeof(clock_t)=%d, CLOCKS_PER_SEC=%d\n",
           sizeof(clock_t), CLOCKS_PER_SEC);
    printf("sizeof(size_t)=%d, sizeof(int)=%d\n\n",
           sizeof(size_t), sizeof(int));

    printf("clock()\n");
    printf("C Native:     %d\n", start);
    printf("C Multiplier: %d\t (x%d)\n", start*TMULTI, TMULTI);
    printf("C Multiplier: %d\t (x%d)\n", start*FMULTI, FMULTI);
    printf("C Multiplier: %d\t (x%d)\n", start*SMULTI, SMULTI);
    printf("C Multiplier: %d\t (x%d)\n", start*OMULTI, OMULTI);
    printf("C Multiplier: %d\t (x%d)\n\n", start*DMULTI, DMULTI);


    printf("One sec sleep: %d - %d = %d\n\n", end, start, end-start);
}
