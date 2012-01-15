/** Complete all-combination generator by Vegard Nossum
 * All bugs are due to fiddling around by Mate Soos
 */
#include <cstdio>
#include <stdint.h>
#include "assert.h"
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

int main(int argc, char *argv[])
{
    assert(argc == 2);
    printf("c argv[1]: %s\n", argv[0]);
    printf("c argv[1]: %s\n", argv[1]);

    const long int nr_vars = strtol(argv[1], NULL, 10);

    if ((errno == ERANGE && (nr_vars == LONG_MAX || nr_vars == LONG_MIN))
           || (errno != 0 && nr_vars == 0)) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }
    if (nr_vars > 62) {
        fprintf(stderr, "Too large value given: %d\n", (int)nr_vars);
        exit(EXIT_FAILURE);
    }

    unsigned long x = 0;
    do {
    for (int i = 0; i < nr_vars; ++i)
        printf("%s%u ", (x >> i) & 1 ? "-" : "", 1 + i);

        printf("0\n");
    } while ((++x & ((1UL << nr_vars) - 1)) != 0);

    return 0;
}
