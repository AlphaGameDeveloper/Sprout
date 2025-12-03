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

#include "Logger.h"

namespace logger
{
void logImpl(Level level, const char* text, const char* file, int line, const char* function)
{
    logNumber++;
    char buffer[WHEEZER_MAX_LOG_LENGTH];

    snprintf(buffer, sizeof(buffer), "\r[%d] %s:%s:%d [%s]  %s", (int)millis(), file, function,
             line, levelToString(level), text);

    Serial.println(buffer);
}

void logImplV(Level level, const char* file, int line, const char* function, const char* fmt,
              va_list args)
{
    char msg[WHEEZER_MAX_LOG_LENGTH];
    vsnprintf(msg, sizeof(msg), fmt, args);
    logImpl(level, msg, file, line, function);
}

void logImplF(Level level, const char* file, int line, const char* function, const char* fmt, ...)
{
    char msg[WHEEZER_MAX_LOG_LENGTH];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    logImpl(level, msg, file, line, function);
}
} // namespace logger