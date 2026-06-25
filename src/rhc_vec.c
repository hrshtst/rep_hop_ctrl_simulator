#include "rhc_vec.h"

static vec_t _vec_set_elem_vlist(vec_t v, va_list args);
vec_t _vec_set_elem_vlist(vec_t v, va_list args)
{
  register size_t i;

  for( i=0; i<vec_size(v); i++ )
    vec_set_elem( v, i, (double)va_arg( args, double ) );
  return v;
}

vec_t vec_set_elem_list(vec_t v, ... )
{
  va_list args;

  va_start( args, v );
  _vec_set_elem_vlist( v, args );
  va_end( args );
  return v;
}

vec_t vec_set_elem_array(vec_t v, double array[])
{
  memcpy( vec_buf( v ), array, sizeof(double)*vec_size( v ) );
  return v;
}

vec_t vec_fill(vec_t v, double val)
{
  register size_t i;

  for( i=0; i<vec_size(v); i++ )
    vec_set_elem( v, i, val );
  return v;
}

vec_t vec_clear(vec_t v)
{
  memset( vec_buf(v), 0, sizeof(double)*vec_size(v) );
  return v;
}

vec_t vec_create(size_t size)
{
  vec_t v;

  if( ( v = nalloc( _vec_t, 1 ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  if( size > 0 ){
    if( ( v->elem = nalloc( double, size ) ) == NULL ){
      ALLOC_ERR();
      free( v );
      return NULL;
    }
  } else
    v->elem = NULL;
  v->size = size;
  return v;
}

vec_t vec_create_list(size_t size, ... )
{
  vec_t v;
  va_list args;

  if( ( v = vec_create( size ) ) == NULL )
    return NULL;
  va_start( args, size );
  _vec_set_elem_vlist( v, args );
  va_end( args );
  return v;
}

vec_t vec_create_array(size_t size, double array[])
{
  vec_t v;

  if( ( v = vec_create( size ) ) == NULL )
    return NULL;
  vec_set_elem_array( v, array );
  return v;
}

void vec_destroy(vec_t v)
{
  if( !v ) return;
  sfree( vec_buf(v) );
  free( v );
}

vec_t vec_copy(vec_t src, vec_t dst)
{
  size_t i;

  if( vec_size(src) != vec_size(dst) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return NULL;
  }
  for( i=0; i<vec_size(src); i++ )
    vec_set_elem( dst, i, vec_elem(src,i) );
  return dst;
}

vec_t vec_clone(vec_t src)
{
  vec_t cln;

  if( ( cln = vec_create( vec_size(src) ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  return vec_copy( src, cln );
}

vec_t vec_add(vec_t v1, vec_t v2, vec_t v)
{
  size_t i;

  if( vec_size(v1) != vec_size(v2) || vec_size(v2) != vec_size(v) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return NULL;
  }
  for( i=0; i<vec_size(v1); i++ )
    vec_set_elem( v, i, vec_elem(v1,i) + vec_elem(v2,i) );
  return v;
}

vec_t vec_sub(vec_t v1, vec_t v2, vec_t v)
{
  size_t i;

  if( vec_size(v1) != vec_size(v2) || vec_size(v2) != vec_size(v) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return NULL;
  }
  for( i=0; i<vec_size(v1); i++ )
    vec_set_elem( v, i, vec_elem(v1,i) - vec_elem(v2,i) );
  return v;
}

vec_t vec_mul(vec_t v1, double k, vec_t v)
{
  size_t i;

  if( vec_size(v1) != vec_size(v) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return NULL;
  }
  for( i=0; i<vec_size(v1); i++ )
    vec_set_elem( v, i, vec_elem(v1,i) * k );
  return v;
}

vec_t vec_div(vec_t v1, double k, vec_t v)
{
  size_t i;

  if( vec_size(v1) != vec_size(v) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return NULL;
  }
  if( istiny( k ) ){
    RUNTIME_ERR( ERR_ZERODIV );
    return NULL;
  }
  for( i=0; i<vec_size(v1); i++ )
    vec_set_elem( v, i, vec_elem(v1,i) / k );
  return v;
}

vec_t vec_cat(vec_t v1, double k, vec_t v2, vec_t v)
{
  size_t i;

  if( vec_size(v1) != vec_size(v2) || vec_size(v2) != vec_size(v) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return NULL;
  }
  for( i=0; i<vec_size(v1); i++ )
    vec_set_elem( v, i, vec_elem(v1,i) + k*vec_elem(v2,i) );
  return v;
}

bool vec_match(vec_t v1, vec_t v2)
{
  register int i;

  if( vec_size(v1) != vec_size(v2) ) {
    RUNTIME_ERR( ERR_SIZMIS );
    return false;
  }
  for( i=0; i<vec_size(v1); i++ )
    if( vec_elem( v1, i ) != vec_elem( v2, i ) )
      return false;
  return true;
}

bool vec_equal(vec_t v1, vec_t v2)
{
  return vec_near( v1, v2, TOL );
}

bool vec_near(vec_t v1, vec_t v2, double tol)
{
  register int i;

  if( vec_size(v1) != vec_size(v2) ) {
    RUNTIME_ERR( ERR_SIZMIS );
    return false;
  }
  for( i=0; i<vec_size(v1); i++ )
    if( !istol( vec_elem(v1,i) - vec_elem(v2,i), tol ) )
      return false;
  return true;
}

double vec_dot(vec_t v1, vec_t v2)
{
  double s = 0;
  register int i;

  if( vec_size(v1) != vec_size(v2) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return 0;
  }
  for( i=0; i<vec_size(v1); i++ )
    s += vec_elem(v1,i) * vec_elem(v2,i);
  return s;
}

double vec_sqr_norm(vec_t v)
{
  return vec_dot( v, v );
}

double vec_sqr_dist(vec_t v1, vec_t v2)
{
  double d = 0;
  register int i;

  if( vec_size(v1) != vec_size(v2) ){
    RUNTIME_ERR( ERR_SIZMIS );
    return 0;
  }
  for( i=0; i<vec_size(v1); i++ )
    d += sqr( vec_elem(v1,i) - vec_elem(v2,i) );
  return d;
}

double vec_cos_sim(vec_t v1, vec_t v2)
{
  return vec_dot( v1, v2 ) / ( vec_norm(v1) * vec_norm(v2) );
}

void vec_f_write(FILE *fp, vec_t v)
{
  register size_t i;

  if( !v ) return;
  for( i=0; i<vec_size(v); i++ )
    fprintf( fp, "%.10g ", vec_elem(v,i) );
  fprintf( fp, "\n" );
}
