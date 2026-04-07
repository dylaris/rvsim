int main(void)
{
    volatile long i;
    for (i = 0; i < 100000000; i++);
    return 0;
}
