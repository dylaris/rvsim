#include <stdio.h>

#include "sqlite3.h"

int main(void)
{
    sqlite3 *db;
    int rc = sqlite3_open(":memory:", &db);
    if (rc != 0) {
        printf("open failed\n");
        return 1;
    }
    printf("SQLite GOGOGO!\n");
    sqlite3_close(db);
    return 0;
}
