/****************************************************************************

  Copyright (c) 2021 Ronald Sieber

  Project:      Generic / Project independent
  Description:  Implementation of DEBUG TRACE

  -------------------------------------------------------------------------

  Revision History:

  2021/01/22 -rs:   V1.00 Initial version

****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>





//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#ifndef STDIN_FILENO
    #define STDIN_FILENO    0
#endif

#ifndef STDOUT_FILENO
    #define STDOUT_FILENO   1
#endif



//---------------------------------------------------------------------------
// trace
//---------------------------------------------------------------------------


#if !defined(NDEBUG)

void  trace (const char* pszFmt_p, ...)
{

va_list  pArgList;


    va_start (pArgList, pszFmt_p);
    vfprintf (stdout, pszFmt_p, pArgList);
    va_end   (pArgList);

    fflush  (stdout);
    tcdrain (STDOUT_FILENO);

}

#endif



// EOF


