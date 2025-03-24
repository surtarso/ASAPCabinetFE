#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>

#ifdef DEBUG_LOGGING
#define LOG_DEBUG(x) do { std::cout << (x); std::cout << std::endl; } while (0)
#else
#define LOG_DEBUG(x) do {} while (0) // No-op in release builds
#endif

#endif // LOGGING_H