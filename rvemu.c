#include "machine.h"
#include "utils.h"

int main(int argc, char **argv)
{
    Machine m;

    if (argc != 2)
        fatalf("Usage: %s <program>", argv[0]);

    machine_load_program(&m, argv[1]);

    return 0;
}
