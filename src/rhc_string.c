#include "rhc_string.h"
#include <ctype.h>

char *string_copy(const char *s, char *d) {
  return strcpy(d, s);
}

int string_len(const char *s) {
  return strlen( s );
}

char *string_reverse(const char *s, char *d) {
  int i;
  int len;

  len = string_len( s );
  for( i=0; i<len; i++ )
    d[i] = s[len - i - 1];
  d[len] = '\0';
  return d;
}

bool string_is_digit(const char *s) {
  int i;

  for( i=0; i<string_len(s); i++ )
    if( isalpha( s[i] ) )
      return false;
  return true;
}

bool string_starts_with(const char *s, const char *prefix) {
  int i;

  for( i=0; i<string_len( prefix ); i++ )
    if( s[i] != prefix[i] )
      return false;
  return true;
}

bool string_ends_with(const char *s, const char *suffix) {
  char srev[BUFSIZ];
  char suffixrev[BUFSIZ];

  /* string_reverse writes len+1 bytes into the local buffers; bail out
     rather than overflow them when either input is too long. */
  if( string_len(s) >= BUFSIZ || string_len(suffix) >= BUFSIZ )
    return false;
  return string_starts_with(
      string_reverse(s, srev), string_reverse(suffix, suffixrev));
}
