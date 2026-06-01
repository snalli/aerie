/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

// Miscellanous classes.

#include <execinfo.h>
#include <stdio.h>

#include "util/types.h"
#include "util/debug.h"

using namespace std;

namespace util
{
// ---------------------------- IMPLEMENTATION ---------------------------- //
//

     string backtrace()
    {
        int32 const NUM_PTRS = 10000;
        void* ptrBuf[NUM_PTRS];

        int32 numPtrs = ::backtrace(ptrBuf, NUM_PTRS);

        if (!numPtrs)
        {
            return "<backtrace not available>";
        }

        char** funcArray = ::backtrace_symbols(ptrBuf, numPtrs);

        if (!funcArray)
        {
            return "<backtrace not available>";
        }

        string result;

        for (int32 i = 1; i < numPtrs; i++)
        { 
            string func = funcArray[i];

            if (func[0] != '[')
            {
                result += func + "\n";
            }
        }
	
		free(funcArray);
	
		return result;
    }
}
