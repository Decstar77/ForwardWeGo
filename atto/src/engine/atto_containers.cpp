#include "atto_containers.h"

#include <string>
#include <stdarg.h>

namespace atto
{
    LargeString ToString( bool v ) {
        LargeString res = {};
        if ( v == true ) {
            res.Add( "true" );
        } else {
            res.Add( "false" );
        }
        return res;
    }

    LargeString ToString( i32 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    LargeString ToString( i64 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    LargeString ToString( u8 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    LargeString ToString( u32 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    LargeString ToString( u64 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    LargeString ToString( f32 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    LargeString ToString( f64 v ) {
        LargeString res = {};
        res.Add( std::to_string( v ).c_str() );
        return res;
    }

    static void StringFormatV(char* dest, size_t size, const char* format, va_list va_listp) {
        vsnprintf(dest, size, format, va_listp);
    }

    SmallString StringFormat::Small(const char* format, ...) {
        SmallString result = {};

        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(result.GetCStr(), result.CAPCITY, format, arg_ptr);
        va_end(arg_ptr);

        result.CalculateLength();

        return result;
    }

    LargeString StringFormat::Large(const char* format, ...) {
        LargeString result = {};

        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(result.GetCStr(), result.CAPCITY, format, arg_ptr);
        va_end(arg_ptr);

        result.CalculateLength();

        return result;
    }

}