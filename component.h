#include "bus.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef COMP_NAME
#error "use `#define COMP_NAME <component name>` to name a component"
#endif

#define COMBINE(a, b) a##_##b

#ifdef COMP_NO_INIT
#undef COMP_NO_INIT
#else
#define COMP_INIT_FUNC(x) bool COMBINE(x, init)(COMP_INIT_ARGS); bool COMBINE(x, destroy)();
COMP_INIT_FUNC(COMP_NAME)
#undef COMP_INIT_FUNC
#undef COMP_INIT_ARGS
#endif

#ifdef COMP_NO_READ
#undef COMP_NO_READ
#else
#define COMP_READ_FUNC(x) uint8_t COMBINE(x, read)(addr_t addr);
COMP_READ_FUNC(COMP_NAME)
#undef COMP_READ_FUNC
#endif

#ifdef COMP_NO_WRITE
#undef COMP_NO_WRITE
#else
#define COMP_WRITE_FUNC(x) void COMBINE(x, write)(addr_t addr, uint8_t data);
COMP_WRITE_FUNC(COMP_NAME)
#undef COMP_WRITE_FUNC
#endif

#undef COMBINE
#undef COMP_NAME
