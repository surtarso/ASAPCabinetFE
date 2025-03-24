#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>

#ifdef DEBUG_LOGGING
#define LOG_DEBUG(x) do { std::cout << x << std::endl; } while (0)
#else
#define LOG_DEBUG(x) do { } while (0)
#endif

#endif // LOGGING_H