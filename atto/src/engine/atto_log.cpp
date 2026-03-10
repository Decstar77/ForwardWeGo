/*
    Atto Engine - Logging System Implementation
*/

#include "atto_log.h"
#include <cstdio>
#include <ctime>

#if ATTO_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
#endif

namespace atto {

    // Console color codes for Windows
    #if ATTO_PLATFORM_WINDOWS
        static constexpr WORD kColorTrace = FOREGROUND_INTENSITY;                                      // Gray
        static constexpr WORD kColorDebug = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; // Cyan
        static constexpr WORD kColorInfo  = FOREGROUND_GREEN | FOREGROUND_INTENSITY;                   // Green
        static constexpr WORD kColorWarn  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  // Yellow
        static constexpr WORD kColorError = FOREGROUND_RED | FOREGROUND_INTENSITY;                     // Red
        static constexpr WORD kColorFatal = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;   // Magenta
        static constexpr WORD kColorReset = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;       // White
    #endif

    Logger & Logger::Get() {
        static Logger instance;
        return instance;
    }

    Logger::~Logger() {
        Shutdown();
    }

    void Logger::Initialize( const LogConfig & cfg ) {
        if ( initialized ) {
            return;
        }

        config = cfg;

        #if ATTO_DEBUG
            minLevel = LogLevel::Trace;
        #else
            minLevel = LogLevel::Info;
        #endif

        if ( config.enableFile && config.logFilePath ) {
            #if ATTO_PLATFORM_WINDOWS
                fopen_s( reinterpret_cast<FILE **>( &fileHandle ), config.logFilePath, "w" );
            #else
                fileHandle = fopen( config.logFilePath, "w" );
            #endif

            if ( !fileHandle ) {
                // Can't use logging here since we're initializing, just print directly
                fprintf( stderr, "[Logger] Failed to open log file: %s\n", config.logFilePath );
            }
        }

        initialized = true;

        ATTO_LOG_INFO( "Logger initialized" );
    }

    void Logger::Shutdown() {
        if ( !initialized ) {
            return;
        }

        ATTO_LOG_INFO( "Logger shutting down" );

        if ( fileHandle ) {
            fclose( static_cast<FILE *>( fileHandle ) );
            fileHandle = nullptr;
        }

        initialized = false;
    }

    void Logger::EnableFile( bool enable ) {
        if ( enable && !fileHandle && config.logFilePath ) {
            #if ATTO_PLATFORM_WINDOWS
                fopen_s( reinterpret_cast<FILE **>( &fileHandle ), config.logFilePath, "a" );
            #else
                fileHandle = fopen( config.logFilePath, "a" );
            #endif
        } else if ( !enable && fileHandle ) {
            fclose( static_cast<FILE *>( fileHandle ) );
            fileHandle = nullptr;
        }
        config.enableFile = enable;
    }

    void Logger::Log( LogLevel level, const char * file, i32 line, const char * fmt, ... ) {
        va_list args;
        va_start( args, fmt );
        LogV( level, file, line, fmt, args );
        va_end( args );
    }

    void Logger::LogV( LogLevel level, const char * file, i32 line, const char * fmt, va_list args ) {
        if ( static_cast<u8>( level ) < static_cast<u8>( minLevel ) ) {
            return;
        }

        // Format the user message
        char userMessage[2048];
        vsnprintf( userMessage, sizeof( userMessage ), fmt, args );

        // Build the full log message
        char fullMessage[4096];
        char * ptr = fullMessage;
        usize remaining = sizeof( fullMessage );

        // Timestamp
        if ( config.showTimestamp ) {
            time_t now = time( nullptr );
            tm localTime;
            #if ATTO_PLATFORM_WINDOWS
                localtime_s( &localTime, &now );
            #else
                localtime_r( &now, &localTime );
            #endif

            int written = snprintf( ptr, remaining, "[%02d:%02d:%02d] ",
                localTime.tm_hour, localTime.tm_min, localTime.tm_sec );
            ptr += written;
            remaining -= written;
        }

        // Level
        int written = snprintf( ptr, remaining, "[%s] ", GetLevelString( level ) );
        ptr += written;
        remaining -= written;

        // Source location
        if ( file && line > 0 ) {
            // Extract just the filename from the path
            const char * filename = file;
            const char * lastSlash = file;
            while ( *file ) {
                if ( *file == '/' || *file == '\\' ) {
                    lastSlash = file + 1;
                }
                file++;
            }
            filename = lastSlash;

            written = snprintf( ptr, remaining, "(%s:%d) ", filename, line );
            ptr += written;
            remaining -= written;
        }

        // User message
        snprintf( ptr, remaining, "%s", userMessage );

        // Output
        if ( config.enableConsole ) {
            WriteToConsole( level, fullMessage );
        }

        if ( config.enableFile && fileHandle ) {
            WriteToFile( fullMessage );
        }
    }

    void Logger::WriteToConsole( LogLevel level, const char * message ) {
        #if ATTO_PLATFORM_WINDOWS
            if ( config.enableColors ) {
                SetConsoleColor( level );
            }

            // Use OutputDebugString for Visual Studio output window
            OutputDebugStringA( message );
            OutputDebugStringA( "\n" );

            // Also print to console
            printf( "%s\n", message );

            if ( config.enableColors ) {
                ResetConsoleColor();
            }
        #else
            // ANSI color codes for Unix-like systems
            if ( config.enableColors ) {
                const char * colorCode = "";
                switch ( level ) {
                    case LogLevel::Trace: colorCode = "\033[90m"; break;  // Gray
                    case LogLevel::Debug: colorCode = "\033[36m"; break;  // Cyan
                    case LogLevel::Info:  colorCode = "\033[32m"; break;  // Green
                    case LogLevel::Warn:  colorCode = "\033[33m"; break;  // Yellow
                    case LogLevel::Error: colorCode = "\033[31m"; break;  // Red
                    case LogLevel::Fatal: colorCode = "\033[35m"; break;  // Magenta
                }
                printf( "%s%s\033[0m\n", colorCode, message );
            } else {
                printf( "%s\n", message );
            }
        #endif

        fflush( stdout );
    }

    void Logger::WriteToFile( const char * message ) {
        if ( fileHandle ) {
            fprintf( static_cast<FILE *>( fileHandle ), "%s\n", message );
            fflush( static_cast<FILE *>( fileHandle ) );
        }
    }

    const char * Logger::GetLevelString( LogLevel level ) {
        switch ( level ) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default:              return "?????";
        }
    }

    void Logger::SetConsoleColor( LogLevel level ) {
        #if ATTO_PLATFORM_WINDOWS
            HANDLE console = GetStdHandle( STD_OUTPUT_HANDLE );
            WORD color = kColorReset;

            switch ( level ) {
                case LogLevel::Trace: color = kColorTrace; break;
                case LogLevel::Debug: color = kColorDebug; break;
                case LogLevel::Info:  color = kColorInfo;  break;
                case LogLevel::Warn:  color = kColorWarn;  break;
                case LogLevel::Error: color = kColorError; break;
                case LogLevel::Fatal: color = kColorFatal; break;
            }

            SetConsoleTextAttribute( console, color );
        #endif
    }

    void Logger::ResetConsoleColor() {
        #if ATTO_PLATFORM_WINDOWS
            HANDLE console = GetStdHandle( STD_OUTPUT_HANDLE );
            SetConsoleTextAttribute( console, kColorReset );
        #endif
    }

} // namespace atto
