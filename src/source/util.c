
#include <stddef.h>
#include <string.h>

char const* uint_binary_str(unsigned int number) {
	static char b[sizeof number * 8 + 1];

	memset(b, 0, sizeof b);
	for (size_t i = 0; i < sizeof number * 8; ++i) {
		unsigned int mask = 1 << i;
		if (number & mask)
			b[sizeof number * 8 - 1 - i] = '1';
		else
			b[sizeof number * 8 - 1 - i] = '0';
	}
	return b;
}
