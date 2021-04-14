#include <stdio.h>

int main()
{
    int i;
    i=0;
    do
    {
        printf("%d %c \n",i,i);
        i++;
    }
    while(i<=255);
    return 0;
}
