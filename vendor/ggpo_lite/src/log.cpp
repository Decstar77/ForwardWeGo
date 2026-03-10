/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "types.h"
#include <stdarg.h>
#include <iostream>

typedef void( __cdecl * log_msg )( const char * msg );

static log_msg LogMsg = NULL;
extern void SetLogCallback( void( __cdecl * _log_msg )( const char * msg ) ) {
    LogMsg = _log_msg;
}

void Log(const char *fmt, ...) {

    char buf[ 1024 ] = {};
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buf, ARRAY_SIZE(buf), fmt, args);
    va_end(args);

    if( LogMsg ) {
        LogMsg( buf );
    }
}


