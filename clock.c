#include "clock.h"

#include "cpu.h"

#include <time.h>
#include <stdio.h>

void clock_reset() {
	cpu_reset(true);
	cpu_clock();
	cpu_reset(false);
}

bool clock_running = false;
void clock_run(long targetFrequency) {
	clock_t prev = clock();
	clock_t diff = CLOCKS_PER_SEC / targetFrequency;
	printf("diff: %ld\n", diff);

	clock_running = true;
	while (clock_running) {
		cpu_clock();
		static int clockCounter = 0;
		if (clockCounter++ % targetFrequency == 0)
			printf("clock\n");

		clock_t now;
		do {
			now = clock();
		} while (now - prev <= diff);
		prev += diff;
	}
}