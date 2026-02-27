#include "clock.h"

#include "cpu.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#ifdef _WIN32
LARGE_INTEGER frequency;
#endif

bool clock_running = false;

uint64_t getTime_us() {
#ifdef _WIN32
	LARGE_INTEGER counter;

	QueryPerformanceCounter(&counter);
	return (uint64_t) ((counter.QuadPart * 1000000ULL) / frequency.QuadPart);
#else
	// for now assume clock_t has microsecond precision
	// wait that is the entire reason i needed to rewrite
	// (my) windows doesnt have microsecond precision
	return (uint64_t) clock();
#endif
}

void clock_reset() {
	cpu_reset(true);
	cpu_clock();
	cpu_reset(false);
}

void clock_run(uint64_t targetFrequency) {
#ifdef _WIN32
	QueryPerformanceFrequency(&frequency);
#endif

	uint64_t prev = getTime_us();
	uint64_t now = prev;
	uint64_t diff = 1000000 / targetFrequency;

	printf("diff: %zi\n", diff);

	clock_running = true;
	while (clock_running) {
		cpu_clock();

		static int clockCounter = 0;
		if (clockCounter++ % targetFrequency == 0)
			printf("clock\nnow: %zi, prev: %zi\n", now, prev);

		do {
			now = getTime_us();
		} while (now - prev <= diff);
		prev += diff;
	}
}
