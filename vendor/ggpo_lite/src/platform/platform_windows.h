/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _GGPO_WINDOWS_H_
#define _GGPO_WINDOWS_H_

#include <stdio.h>
#include <memory>
#include "../types.h"

class Platform {
public:
   typedef uint32 ProcessID;

public:
   static ProcessID GetProcessID();
   static void AssertFailed(char *msg);
   static uint32 GetCurrentTimeMS();
   static int GetConfigInt(const char* name);
   static bool GetConfigBool(const char* name);
};

#endif
