#ifndef __RHC_DEFS_H__
#define __RHC_DEFS_H__

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
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

/* bool/true/false come from <stdbool.h>; size_t from <stddef.h>. */

#endif /* __RHC_DEFS_H__ */
