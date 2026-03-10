#pragma once

/*
    Atto Engine - Logging System
    Provides compile-time toggleable logging with multiple severity levels
*/

#include "atto_core.h"
#include <cstdarg>

// Log level configuration
// Define ATTO_LOG_LEVEL before including this header to set minimum log level
// 0 = All, 1 = Debug+, 2 = Info+, 3 = Warn+, 4 = Error+, 5 = Fatal only, 6 = None
#ifndef ATTO_LOG_LEVEL
    #if ATTO_DEBUG
        #define ATTO_LOG_LEVEL 0  // Show all logs in debug
    #else
        #define ATTO_LOG_LEVEL 3  // Info and above in release
    #endif
#endif

// Enable/disable logging entirely
#ifndef ATTO_LOGGING_ENABLED
    #define ATTO_LOGGING_ENABLED 1
#endif

// Enable file/line info in log messages
#ifndef ATTO_LOG_SHOW_SOURCE
    #if ATTO_DEBUG
        #define ATTO_LOG_SHOW_SOURCE 1
    #else
        #define ATTO_LOG_SHOW_SOURCE 0
    #endif
#endif

namespace atto {

    enum class LogLevel : u8 {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Fatal = 5
    };

    // Logger configuration
    struct LogConfig {
        bool enableConsole = true;
        bool enableFile = false;
        bool enableColors = true;
        bool showTimestamp = true;
        const char * logFilePath = "atto.log";
    };

    class Logger {
    public:
        static Logger & Get();

        void Initialize( const LogConfig & config = LogConfig() );
        void Shutdown();

        void Log( LogLevel level, const char * file, i32 line, const char * fmt, ... );
        void LogV( LogLevel level, const char * file, i32 line, const char * fmt, va_list args );

        void SetMinLevel( LogLevel level ) { minLevel = level; }
        LogLevel GetMinLevel() const { return minLevel; }

        void EnableConsole( bool enable ) { config.enableConsole = enable; }
        void EnableFile( bool enable );
        void EnableColors( bool enable ) { config.enableColors = enable; }

    private:
        Logger() = default;
        ~Logger();
        Logger( const Logger & ) = delete;
        Logger & operator=( const Logger & ) = delete;

        void WriteToConsole( LogLevel level, const char * message );
        void WriteToFile( const char * message );
        const char * GetLevelString( LogLevel level );
        void SetConsoleColor( LogLevel level );
        void ResetConsoleColor();

        LogConfig config;
        LogLevel minLevel = LogLevel::Trace;
        void * fileHandle = nullptr;
        bool initialized = false;
    };

} // namespace atto

// =============================================================================
// Logging Macros
// =============================================================================

#if ATTO_LOGGING_ENABLED

    #if ATTO_LOG_SHOW_SOURCE
        #define ATTO_LOG_IMPL(level, fmt, ...) \
            ::atto::Logger::Get().Log(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_IMPL(level, fmt, ...) \
            ::atto::Logger::Get().Log(level, nullptr, 0, fmt, ##__VA_ARGS__)
    #endif

    // Trace - Very detailed debugging info (compiled out in release)
    #if ATTO_LOG_LEVEL <= 0
        #define ATTO_LOG_TRACE(fmt, ...) ATTO_LOG_IMPL(::atto::LogLevel::Trace, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_TRACE(fmt, ...) ((void)0)
    #endif

    // Debug - Debugging info (compiled out in release)
    #if ATTO_LOG_LEVEL <= 1
        #define ATTO_LOG_DEBUG(fmt, ...) ATTO_LOG_IMPL(::atto::LogLevel::Debug, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_DEBUG(fmt, ...) ((void)0)
    #endif

    // Info - General information
    #if ATTO_LOG_LEVEL <= 2
        #define ATTO_LOG_INFO(fmt, ...) ATTO_LOG_IMPL(::atto::LogLevel::Info, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_INFO(fmt, ...) ((void)0)
    #endif

    // Warn - Warnings
    #if ATTO_LOG_LEVEL <= 3
        #define ATTO_LOG_WARN(fmt, ...) ATTO_LOG_IMPL(::atto::LogLevel::Warn, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_WARN(fmt, ...) ((void)0)
    #endif

    // Error - Errors
    #if ATTO_LOG_LEVEL <= 4
        #define ATTO_LOG_ERROR(fmt, ...) ATTO_LOG_IMPL(::atto::LogLevel::Error, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_ERROR(fmt, ...) ((void)0)
    #endif

    // Fatal - Fatal errors (always enabled unless logging is completely disabled)
    #if ATTO_LOG_LEVEL <= 5
        #define ATTO_LOG_FATAL(fmt, ...) ATTO_LOG_IMPL(::atto::LogLevel::Fatal, fmt, ##__VA_ARGS__)
    #else
        #define ATTO_LOG_FATAL(fmt, ...) ((void)0)
    #endif

#else
    // Logging completely disabled
    #define ATTO_LOG_TRACE(fmt, ...) ((void)0)
    #define ATTO_LOG_DEBUG(fmt, ...) ((void)0)
    #define ATTO_LOG_INFO(fmt, ...)  ((void)0)
    #define ATTO_LOG_WARN(fmt, ...)  ((void)0)
    #define ATTO_LOG_ERROR(fmt, ...) ((void)0)
    #define ATTO_LOG_FATAL(fmt, ...) ((void)0)
#endif

// Shorter aliases (optional, can be disabled by defining ATTO_LOG_NO_SHORT_MACROS)
#ifndef ATTO_LOG_NO_SHORT_MACROS
    #define LOG_TRACE ATTO_LOG_TRACE
    #define LOG_DEBUG ATTO_LOG_DEBUG
    #define LOG_INFO  ATTO_LOG_INFO
    #define LOG_WARN  ATTO_LOG_WARN
    #define LOG_ERROR ATTO_LOG_ERROR
    #define LOG_FATAL ATTO_LOG_FATAL
#endif
