#ifndef IAPP_H
#define IAPP_H

/**
 * @file iapp.h
 * @brief Defines a simple interface for modular app entry points.
 *
 * The IApp interface allows multiple application modules (e.g., Frontend, Editor)
 * to be hosted within the same executable and launched via command-line flags.
 */
class IApp {
public:
    virtual ~IApp() = default;

    /**
     * @brief Run the application's main loop.
     */
    virtual void run() = 0;
};

#endif // IAPP_H
