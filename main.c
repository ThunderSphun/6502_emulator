#include "bus.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main() {
	bus_t bus;
	bus_init(&bus);
	bus_print(&bus);
	printf("%p\n", bus.addresses);
	bus_destroy(&bus);
	return 0;
}