#include "util.h"

#include <string.h>

const char* byteToBinStr(const uint8_t byte) {
	static char str[9] = { 0 };
	str[0] = 0;

	for (int i = 7; i >= 0; i--)
#ifdef _MSC_VER
		strcat_s(str, sizeof(str) / sizeof(str[0]), (byte & (1 << i)) ? "1" : "0");
#else
		strcat(str, (byte & (1 << i)) ? "1" : "0");
#endif

	return str;
}
