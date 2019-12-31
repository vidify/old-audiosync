#include <stdio.h>
#include <stdlib.h>
#include <vidify_audiosync/audiosync.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"TITLE\"\n", argv[0]);
        exit(1);
    }

    printf("Running get_lag\n");
    int ret = get_lag(argv[1]);
    printf("Returned value: %d\n", ret);

    return 0;
}
