#include <stdio.h>

int main(void)
{
    volatile long i;
    for (i = 0; i < 100000000; i++);
    printf("%ld\n", i);
    return 0;
}
