#ifndef __RHC_DEFS_H__
#define __RHC_DEFS_H__

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project-wide buffer size for messages, tags, filenames, etc.
   Defined explicitly rather than reusing stdio's RHC_BUFSIZ, whose value
   is platform-dependent (and which made the previous `#ifndef RHC_BUFSIZ`
   override a silent no-op, since <stdio.h> always defines it first). */
#define RHC_BUFSIZ 8192

/* error messages */
#define ERR_SIZMIS        "size mismatch of vector"
#define ERR_ZERODIV       "division by zero"
#define ERR_BUF_EXHAUSTED "exhausted buffer for string"

#ifndef __USE_MISC
typedef unsigned int       uint;
typedef unsigned long int  ulong;
typedef unsigned short int ushort;
#endif
typedef signed char        byte;
typedef unsigned char      ubyte;
typedef signed short int   word;
typedef unsigned short int uword;

#if !(defined (__GNUG__) && defined (size_t))
typedef ulong size_t;
#endif

typedef ubyte bool;
enum{
  false = 0, true = 1
};


#endif /* __RHC_DEFS_H__ */
