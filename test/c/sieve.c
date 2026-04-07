#define SIZE 1000000

int main(void)
{
    char flags[SIZE];
    int i, j;

    for (i = 0; i < SIZE; i++)
        flags[i] = 1;

    for (i = 2; i * i < SIZE; i++) {
        if (flags[i]) {
            for (j = i * i; j < SIZE; j += i)
                flags[j] = 0;
        }
    }
    return 0;
}
