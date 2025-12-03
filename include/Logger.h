/**
 *   Copyright © 2025 Dose Devices
 *   All Rights Reserved.
 *
 *   This source code is proprietary and confidential.
 *   Unauthorized copying, distribution, modification, or use of this file,
 *   via any medium, is strictly prohibited.
 *
 *   Access to this code is granted only to authorized individuals explicitly
 *   approved by Dose Devices. Any use outside the scope of that authorization
 *   is forbidden.
 */

#ifndef WHEEZER_INIT
#define WHEEZER_INIT

#ifdef CLANGD_WORKAROUND
#define XCHAL_INSTROM0_VADDR
#endif

#include <Arduino.h>
#include <cstdarg>
#include <cstdio>

#if defined(ESP8266)
#include "core_esp8266_features.h"
#elif defined(ESP32)
// On ESP32 this header isn't available; include required Arduino/esp headers instead
#include <esp_task_wdt.h>
#endif

#define WHEEZER_MAX_LOG_LENGTH 255

namespace logger
{

inline long int logNumber = 0;

/**
 * Log level definitions using strongly-typed enum class for type safety
 */
enum class Level
{
    Debug   = 0,
    Verbose = 1,
    Info    = 2,
    Warning = 3,
    Error   = 4,
    Wtf     = 5,
};

/**
 * Convert a log level to its string representation
 */
inline const char* levelToString(Level level)
{
    switch (level)
    {
        case Level::Debug:
            return "DEBUG";
        case Level::Verbose:
            return "VERBOSE";
        case Level::Info:
            return "INFO";
        case Level::Warning:
            return "WARNING";
        case Level::Error:
            return "ERROR";
        case Level::Wtf:
            return "WTF";
        default:
            return "UNKNOWN";
    }
}

/**
 * Internal implementation: log a raw C-string message with source location
 */
void logImpl(Level level, const char* text, const char* file, int line, const char* function);

/**
 * Internal implementation: log a formatted message with source location (va_list version)
 */
void logImplV(Level level, const char* file, int line, const char* function, const char* fmt,
              va_list args);

/**
 * Internal implementation: log a formatted message with source location
 */
void logImplF(Level level, const char* file, int line, const char* function, const char* fmt, ...);

/**
 * Log a debug message with source location
 * @param msg The message to log
 * @param file Source file (automatically captured via macro)
 * @param line Source line (automatically captured via macro)
 * @param func Source function (automatically captured via macro)
 */
inline void debug(const char* msg, const char* file, int line, const char* func)
{
    logImpl(Level::Debug, msg, file, line, func);
}

/**
 * Log a verbose message with source location
 */
inline void verbose(const char* msg, const char* file, int line, const char* func)
{
    logImpl(Level::Verbose, msg, file, line, func);
}

/**
 * Log an info message with source location
 */
inline void info(const char* msg, const char* file, int line, const char* func)
{
    logImpl(Level::Info, msg, file, line, func);
}

/**
 * Log a warning message with source location
 */
inline void warning(const char* msg, const char* file, int line, const char* func)
{
    logImpl(Level::Warning, msg, file, line, func);
}

/**
 * Log an error message with source location
 */
inline void error(const char* msg, const char* file, int line, const char* func)
{
    logImpl(Level::Error, msg, file, line, func);
}

/**
 * Log a WTF (What a Terrible Failure) message with source location
 */
inline void wtf(const char* msg, const char* file, int line, const char* func)
{
    logImpl(Level::Wtf, msg, file, line, func);
}

/**
 * Log a debug message with printf-style formatting
 */
template <typename... Args>
inline void debugf(const char* file, int line, const char* func, const char* fmt, Args... args)
{
    logImplF(Level::Debug, file, line, func, fmt, args...);
}

/**
 * Log a verbose message with printf-style formatting
 */
template <typename... Args>
inline void verbosef(const char* file, int line, const char* func, const char* fmt, Args... args)
{
    logImplF(Level::Verbose, file, line, func, fmt, args...);
}

/**
 * Log an info message with printf-style formatting
 */
template <typename... Args>
inline void infof(const char* file, int line, const char* func, const char* fmt, Args... args)
{
    logImplF(Level::Info, file, line, func, fmt, args...);
}

/**
 * Log a warning message with printf-style formatting
 */
template <typename... Args>
inline void warningf(const char* file, int line, const char* func, const char* fmt, Args... args)
{
    logImplF(Level::Warning, file, line, func, fmt, args...);
}

/**
 * Log an error message with printf-style formatting
 */
template <typename... Args>
inline void errorf(const char* file, int line, const char* func, const char* fmt, Args... args)
{
    logImplF(Level::Error, file, line, func, fmt, args...);
}

/**
 * Log a WTF message with printf-style formatting
 */
template <typename... Args>
inline void wtff(const char* file, int line, const char* func, const char* fmt, Args... args)
{
    logImplF(Level::Wtf, file, line, func, fmt, args...);
}

} // namespace logger

/**
 * @defgroup LoggerMacros Logger Logging Macros
 *
 * These macros automatically capture source location (__FILE__, __LINE__, __func__)
 * and pass them to the logger namespace functions.
 *
 * Usage:
 *   L_INFO("Simple message");
 *   L_INFOF("Formatted message: %d", value);
 * @{
 */

// Simple string logging macros
#define L_DEBUG(msg) logger::debug((msg), __FILE__, __LINE__, __func__)
#define L_VERBOSE(msg) logger::verbose((msg), __FILE__, __LINE__, __func__)
#define L_INFO(msg) logger::info((msg), __FILE__, __LINE__, __func__)
#define L_WARNING(msg) logger::warning((msg), __FILE__, __LINE__, __func__)
#define L_ERROR(msg) logger::error((msg), __FILE__, __LINE__, __func__)
#define L_WTF(msg) logger::wtf((msg), __FILE__, __LINE__, __func__)

// Printf-style formatting macros
#define L_DEBUGF(fmt, ...) logger::debugf(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define L_VERBOSEF(fmt, ...) logger::verbosef(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define L_INFOF(fmt, ...) logger::infof(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define L_WARNINGF(fmt, ...) logger::warningf(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define L_ERRORF(fmt, ...) logger::errorf(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define L_WTFF(fmt, ...) logger::wtff(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/** @} */ // end of WheezerMacros group

#endif
