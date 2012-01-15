#include <cstdio>
#include <cstdint>
#include <random>

int main(int argc, char *argv[])
{
	static const unsigned int nr_vars = 15;

	uint32_t x = 0;
	do {
		for (unsigned int i = 0; i < nr_vars; ++i)
			printf("%s%u ", (x >> i) & 1 ? "-" : "", 1 + i);

		printf("0\n");
	} while ((++x & ((1U << nr_vars) - 1)) != 0);

	return 0;
}
