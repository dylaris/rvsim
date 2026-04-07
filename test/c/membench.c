int main(void)
{
    int arr[1024];
    volatile int sum = 0;

    for (int loop = 0; loop < 10000; loop++) {
        for (int i = 0; i < 1024; i++) {
            arr[i] = i;
            sum += arr[i];
        }
    }
    return sum;
}
