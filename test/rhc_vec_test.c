#include "rhc_test.h"
#include "rhc_vec.h"
#include <malloc.h>

/* set random values to a vector */
void set_vec_rand(vec_t v)
{
  size_t i;

  for( i=0; i<vec_size(v); i++ )
    vec_set_elem( v, i, rand() );
}

/* check if a vector created by 'vec_create' has correct size */
void check_vec_create(size_t n, size_t expected)
{
  vec_t v;

  v = vec_create( n );
  ASSERT_EQ( expected, vec_size( v ) );
  ASSERT_LE( sizeof(double) * expected, malloc_usable_size(v->elem) );
  vec_destroy( v );
}

TEST(test_vec_create)
{
  struct case_t {
    size_t n, expected;
  } cases[] = { {2, 2}, {5, 5}, {0, 0} };
  struct case_t *c;

  for( c=cases; (*c).n != 0; c++ )
    check_vec_create( (*c).n, (*c).expected );
}

TEST(test_vec_create_zero_size)
{
  vec_t v;

  v = vec_create( 0 );
  ASSERT_EQ( 0, vec_size( v ) );
  ASSERT_FALSE( vec_buf( v ) );
  vec_destroy( v );
}

TEST(test_destroy)
{
  vec_t v;

  v = vec_create( 2 );
  vec_destroy( v );
  /* ASSERT_PTREQ( NULL, vec_buf(v) ); */
}

TEST(test_destroy_null)
{
  vec_t v;

  v = NULL;
  vec_destroy( v );
  /* ASSERT_PTREQ( NULL, vec_buf(v) ); */
}

void check_vec_elem(vec_t v, size_t size, ... )
{
  va_list args;
  double val;
  size_t i;

  va_start( args, size );
  for( i=0; i<size; i++ ){
    val = (double)va_arg( args, double );
    ASSERT_EQ( val, vec_elem( v, i ) );
  }
  va_end( args );
}

void check_vec_set_elem(size_t size, double *elem)
{
  vec_t v;
  size_t i;

  v = vec_create( size );
  for( i=0; i<size; i++ )
    vec_set_elem( v, i, elem[i] );
  for( i=0; i<size; i++ )
    ASSERT_EQ( elem[i], vec_elem( v, i ) );
  vec_destroy( v );
}

TEST(test_vec_set_elem)
{
  struct case_t {
    size_t n;
    double elem[10];
  } cases[] = {
    { 1, { 1.0 } },
    { 3, { 2.0, 3.0, 5.5 } },
    { 0, {} }
  };
  struct case_t *c;

  for( c=cases; (*c).n>0; c++ )
    check_vec_set_elem( (*c).n, (*c).elem );
}

TEST(test_vec_set_elem_list)
{
  vec_t v;

  v = vec_create( 3 );
  vec_set_elem_list( v, 1.0, 2.0, 3.0 );
  check_vec_elem( v, 3, 1.0, 2.0, 3.0 );
  vec_destroy( v );

  v = vec_create( 6 );
  vec_set_elem_list( v, -3.0, -2.0, -1.0, 1.0, 2.0, 3.0 );
  check_vec_elem( v, 6, -3.0, -2.0, -1.0, 1.0, 2.0, 3.0 );
  vec_destroy( v );
}

void check_vec_set_elem_array(size_t n, double *elem)
{
  vec_t v;
  size_t i;

  v = vec_create( n );
  vec_set_elem_array( v, elem );
  ASSERT_EQ( n, vec_size( v ) );
  for( i=0; i<n; i++ )
    ASSERT_EQ( elem[i], vec_elem(v, i) );
  vec_destroy( v );
}

TEST(test_vec_set_elem_array)
{
  struct case_t {
    size_t n;
    double elem[10];
  } cases[] = {
    { 3, { 1.0, 2.0, 3.0 } },
    { 6, { 1.0, 2.0, 3.0, -3.0, -2.0, -1.0 } },
    { 0, {} }
  };
  struct case_t *c;

  for( c=cases; (*c).n > 0; c++ )
    check_vec_set_elem_array( (*c).n, (*c).elem );
}

void check_vec_fill(size_t n, double val)
{
  vec_t v;
  size_t i;

  v = vec_create( n );
  set_vec_rand( v );
  vec_fill( v, val );
  for( i=0; i<vec_size(v); i++ )
    ASSERT_EQ( val,vec_elem(v,i) );
  vec_destroy( v );
}

TEST(test_vec_fill)
{
  struct case_t {
    int size;
    double val;
  } cases[] = {
    { 2, 0.1 },
    { 3, -0.5 },
    { 4, 3.0 },
    { 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->size>0; c++ )
    check_vec_fill( c->size, c->val );
}

void check_vec_clear(size_t n)
{
  vec_t v;
  size_t i;

  v = vec_create( n );
  set_vec_rand( v );
  vec_clear( v );
  for( i=0; i<vec_size(v); i++ )
    ASSERT_EQ( 0.0, vec_elem(v,i) );
  vec_destroy( v );
}

TEST(test_vec_clear)
{
  int cases[] = { 3, 5, 0 };

  for( int *c=cases; (*c) > 0; c++ )
    check_vec_clear( *c );
}

TEST(test_vec_create_list)
{
  vec_t v;

  v = vec_create_list( 3, 1.0, 2.0, 3.0 );
  ASSERT_EQ( 3, vec_size( v ) );
  check_vec_elem( v, 3, 1.0, 2.0, 3.0 );
  vec_destroy( v );

  v = vec_create_list( 6, 1.0, 2.0, 3.0, -3.0, -2.0, -1.0 );
  ASSERT_EQ( 6, vec_size( v ) );
  check_vec_elem( v, 6, 1.0, 2.0, 3.0, -3.0, -2.0, -1.0 );
  vec_destroy( v );
}

void check_vec_elem_with_array(double *expected, vec_t v)
{
  size_t i;

  for( i=0; i<vec_size(v); i++ )
    ASSERT_EQ( expected[i], vec_elem( v, i ) );
}

void check_vec_create_array(size_t n, double *elem)
{
  vec_t v;

  v = vec_create_array( n, elem );
  ASSERT_EQ( n, vec_size( v ) );
  check_vec_elem_with_array( elem, v );
  vec_destroy( v );
}

TEST(test_vec_create_array)
{
  struct case_t {
    size_t n;
    double elem[10];
  } cases[] = {
    { 3, { 1.0, 2.0, 3.0 } },
    { 6, { 1.0, 2.0, 3.0, -3.0, -2.0, -1.0 } },
    { 0, {} }
  };
  struct case_t *c;

  for( c=cases; (*c).n > 0; c++ )
    check_vec_create_array( (*c).n, (*c).elem );
}

void check_vec_vec_op(size_t n, double *val1, double *val2, double *expected, vec_t (*method)(vec_t,vec_t,vec_t))
{
  vec_t v1, v2, v;

  v1 = vec_create_array( n, val1 );
  v2 = vec_create_array( n, val2 );
  v  = vec_create( n );

  method( v1, v2, v );

  check_vec_elem_with_array( expected, v );
  vec_destroy( v1 );
  vec_destroy( v2 );
  vec_destroy( v );
}

TEST(test_vec_add)
{
  struct case_t {
    size_t n;
    double val1[10];
    double val2[10];
    double expected[10];
  } cases[] = {
    { 2, { 1.0, 1.0 }, { 3.0, 4.0 }, { 4.0, 5.0 } },
    { 4, { -2.0, 3.0, -4.0, 5.0 }, { -3.0, 4.0, 5.0, -6.0 }, { -5.0, 7.0, 1.0, -1.0 } },
    { 0, {}, {}, {} },
  };
  struct case_t *c;

  for( c=cases; (*c).n > 0; c++ )
    check_vec_vec_op( (*c).n, (*c).val1, (*c).val2, (*c).expected, vec_add );
}

void check_vec_size_2(size_t s1, size_t s2, vec_t (*method)(vec_t,double,vec_t))
{
  vec_t v1, v2, ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  v1 = vec_create( s1 );
  v2 = vec_create( s2 );
  ret = method( v1, 1.0, v2 );
  ASSERT_STREQ( ERR_SIZMIS, __err_last_msg );
  ASSERT_PTREQ( NULL, ret );
  vec_destroy( v1 );
  vec_destroy( v2 );
  ECHO_ON();
}

void check_vec_size_3(size_t s1, size_t s2, size_t s3, vec_t (*method)(vec_t,vec_t,vec_t))
{
  vec_t v1, v2, v3, ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  v1 = vec_create( s1 );
  v2 = vec_create( s2 );
  v3 = vec_create( s3 );
  ret = method( v1, v2, v3 );
  ASSERT_STREQ( ERR_SIZMIS, __err_last_msg );
  ASSERT_PTREQ( NULL, ret );
  vec_destroy( v1 );
  vec_destroy( v2 );
  vec_destroy( v3 );
  ECHO_ON();
}

TEST(test_vec_add_size_mismatch)
{
  struct case_t {
    size_t v1_s, v2_s, v3_s;
  } cases[] = {
    { 2, 3, 3 },
    { 3, 3, 1 },
    { 2, 3, 1 },
    { 0, 0, 0 }
  };
  struct case_t *c;

  for( c=cases; (*c).v1_s>0; c++ )
    check_vec_size_3( (*c).v1_s, (*c).v2_s, (*c).v3_s, vec_add );
}

TEST(test_vec_sub)
{
  struct case_t {
    size_t n;
    double val1[10];
    double val2[10];
    double expected[10];
  } cases[] = {
    { 2, { 1.0, 1.0 }, { 3.0, 4.0 }, { -2.0, -3.0 } },
    { 4, { -2.0, 3.0, -4.0, 5.0 }, { -3.0, 4.0, 5.0, -6.0 }, { 1.0, -1.0, -9.0, 11.0 } },
    { 0, {}, {}, {} },
  };
  struct case_t *c;

  for( c=cases; (*c).n > 0; c++ )
    check_vec_vec_op( (*c).n, (*c).val1, (*c).val2, (*c).expected, vec_sub );
}

TEST(test_vec_sub_size_mismatch)
{
  struct case_t {
    size_t v1_s, v2_s, v3_s;
  } cases[] = {
    { 2, 3, 3 },
    { 3, 3, 1 },
    { 2, 3, 1 },
    { 0, 0, 0 }
  };
  struct case_t *c;

  for( c=cases; (*c).v1_s>0; c++ )
    check_vec_size_3( (*c).v1_s, (*c).v2_s, (*c).v3_s, vec_sub );
}

void check_vec_scalar_op(size_t n, double *val1, double k, double *expected, vec_t (*method)(vec_t,double,vec_t))
{
  vec_t v1, v;

  v1 = vec_create_array( n, val1 );
  v  = vec_create( n );

  method( v1, k, v );

  check_vec_elem_with_array( expected, v );
  vec_destroy( v1 );
  vec_destroy( v );
}

TEST(test_vec_mul)
{
  struct case_t {
    size_t n;
    double val1[10];
    double k;
    double expected[10];
  } cases[] = {
    { 2, { 2.0, 3.0 }, 2.0, { 4.0, 6.0 } },
    { 4, { -3.0, 5.0, -1.0, 10.0 }, 4.0, { -12.0, 20.0, -4.0, 40.0 } },
    { 0, {}, 0, {} }
  };
  struct case_t *c;

  for( c=cases; (*c).n>0; c++ )
    check_vec_scalar_op( (*c).n, (*c).val1, (*c).k, (*c).expected, vec_mul );
}

TEST(test_vec_mul_size_mismatch)
{
  struct case_t {
    size_t v1_s, v2_s;
  } cases[] = {
    { 2, 3 },
    { 0, 0 }
  };
  struct case_t *c;

  for( c=cases; (*c).v1_s>0; c++ )
    check_vec_size_2( (*c).v1_s, (*c).v2_s, vec_mul );
}

TEST(test_vec_div)
{
  struct case_t {
    size_t n;
    double val1[10];
    double k;
    double expected[10];
  } cases[] = {
    { 2, { 2.0, 3.0 }, 2.0, { 1.0, 1.5 } },
    { 4, { -8.0, 16.0, -20.0, 100.0 }, 4.0, { -2.0, 4.0, -5.0, 25.0 } },
    { 0, {}, 0, {} }
  };
  struct case_t *c;

  for( c=cases; (*c).n>0; c++ )
    check_vec_scalar_op( (*c).n, (*c).val1, (*c).k, (*c).expected, vec_div );
}

TEST(test_vec_div_size_mismatch)
{
  struct case_t {
    size_t v1_s, v2_s;
  } cases[] = {
    { 2, 3 },
    { 0, 0 }
  };
  struct case_t *c;

  for( c=cases; (*c).v1_s>0; c++ )
    check_vec_size_2( (*c).v1_s, (*c).v2_s, vec_div );
}

TEST(test_vec_div_by_zero)
{
  vec_t v1, ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  v1 = vec_create_list( 2, 1.0, 1.0 );
  ret = vec_div( v1, 0.0, v1 );
  ASSERT_STREQ( ERR_ZERODIV, __err_last_msg );
  ASSERT_PTREQ( NULL, ret );
  vec_destroy( v1 );
  ECHO_ON();
}

void check_vec_cat(size_t n, double *val1, double k, double *val2, double *expected)
{
  vec_t v1, v2, v;

  v1 = vec_create_array( n, val1 );
  v2 = vec_create_array( n, val2 );
  v  = vec_create( n );

  vec_cat( v1, k, v2, v );

  check_vec_elem_with_array( expected, v );
  vec_destroy( v1 );
  vec_destroy( v2 );
  vec_destroy( v );
}

TEST(test_vec_cat)
{
  struct case_t {
    size_t n;
    double val1[10];
    double k;
    double val2[10];
    double expected[10];
  } cases[] = {
    { 2, { 1.0, -1.0 }, 3, { -3.0, 2.0}, { -8.0, 5.0 } },
    { 3, { 1.0, -1.0, 1.0 }, 2, { -2.0, 2.0, 3.0}, { -3.0, 3.0, 7.0 } },
    { 0, {}, 0, {}, {} }
  };
  struct case_t *c;

  for( c=cases; (*c).n; c++ )
    check_vec_cat( (*c).n, (*c).val1, (*c).k, (*c).val2, (*c).expected );
}

void check_vec_cat_size_mismatch(size_t s1, size_t s2, size_t s3)
{
  vec_t v1, v2, v3, ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  v1 = vec_create( s1 );
  v2 = vec_create( s2 );
  v3 = vec_create( s3 );
  ret = vec_cat( v1, 1.0, v2, v3 );
  ASSERT_STREQ( ERR_SIZMIS, __err_last_msg );
  ASSERT_PTREQ( NULL, ret );
  vec_destroy( v1 );
  vec_destroy( v2 );
  vec_destroy( v3 );
  ECHO_ON();
}

TEST(test_vec_cat_size_mismatch)
{
  struct case_t {
    size_t s1, s2, s3;
  } cases[] = {
    { 2, 3, 3 },
    { 3, 3, 1 },
    { 2, 3, 1 },
    { 0, 0, 0 }
  };
  struct case_t *c;

  for( c=cases; (*c).s1>0; c++ )
    check_vec_cat_size_mismatch( (*c).s1, (*c).s2, (*c).s3 );
}

void check_vec_copy(size_t n, double *val, double *expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n, val );
  v2 = vec_create( n );
  vec_copy( v1, v2 );
  check_vec_elem_with_array( expected, v2 );
  vec_destroy( v2 );
  vec_destroy( v1 );
}

TEST(test_vec_copy)
{
  struct case_t {
    size_t n;
    double val[10];
    double expected[10];
  } cases[] = {
    { 2, { 1.0, 1.0 }, { 1.0, 1.0 } },
    { 4, { -2.0, 3.0, -4.0, 5.0 }, { -2.0, 3.0, -4.0, 5.0 } },
    { 5, { 0.0, 0.0, 3.0, 2.0, -1.0 }, { 0.0, 0.0, 3.0, 2.0, -1.0 } },
    { 0, {}, {} },
  };
  struct case_t *c;

  for( c=cases; (*c).n > 0; c++ )
    check_vec_copy( (*c).n, (*c).val, (*c).expected );
}

void check_vec_copy_size_mismatch(size_t s1, size_t s2)
{
  vec_t v1, v2, ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  v1 = vec_create( s1 );
  v2 = vec_create( s2 );
  ret = vec_copy( v1, v2 );
  ASSERT_STREQ( ERR_SIZMIS, __err_last_msg );
  ASSERT_PTREQ( NULL, ret );
  vec_destroy( v1 );
  vec_destroy( v2 );
  ECHO_ON();
}

TEST(test_vec_copy_size_mismatch)
{
  struct case_t {
    size_t s1, s2;
  } cases[] = {
    { 2, 3 },
    { 4, 3 },
    { 0, 0 }
  };
  struct case_t *c;

  for( c=cases; (*c).s1>0; c++ )
    check_vec_copy_size_mismatch( (*c).s1, (*c).s2 );
}

void check_vec_clone(size_t n, double *val, double *expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n, val );
  v2 = vec_clone( v1 );
  check_vec_elem_with_array( expected, v2 );
  ASSERT_PTRNE( vec_buf(v2), vec_buf(v1) );
  vec_destroy( v2 );
  vec_destroy( v1 );
}

TEST(test_vec_clone)
{
  struct case_t {
    size_t n;
    double val[10];
    double expected[10];
  } cases[] = {
    { 2, { 1.0, 1.0 }, { 1.0, 1.0 } },
    { 4, { -2.0, 3.0, -4.0, 5.0 }, { -2.0, 3.0, -4.0, 5.0 } },
    { 5, { 0.0, 0.0, 3.0, 2.0, -1.0 }, { 0.0, 0.0, 3.0, 2.0, -1.0 } },
    { 0, {}, {} },
  };
  struct case_t *c;

  for( c=cases; (*c).n > 0; c++ )
    check_vec_clone( (*c).n, (*c).val, (*c).expected );
}

void check_vec_match(size_t n1, size_t n2, double *val1, double *val2, bool expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n1, val1 );
  v2 = vec_create_array( n2, val2 );
  if( expected )
    ASSERT_TRUE( vec_match( v1, v2 ) );
  else
    ASSERT_FALSE( vec_match( v1, v2 ) );
  vec_destroy( v1 );
  vec_destroy( v2 );
}

TEST(test_vec_match)
{
  struct case_t {
    size_t n1, n2;
    double val1[10];
    double val2[10];
    bool expected;
  } cases[] = {
    { 2, 2, { 1.0, 1.0 }, { 1.0, 1.0 }, true },
    { 2, 2, { 1.0, 1.0 }, { 2.0, 1.0 }, false },
    { 2, 2, { 1.0, 1.0 }, { 1.0, 2.0 }, false },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, 2.0, 3.0 }, true },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 3.0, 2.0, 1.0 }, false },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, sqrt(2)*sqrt(2), 3.0 }, false },
    { 2, 4, { 1.0, 2.0 }, { 1.0, 2.0, 3.0, 4.0 }, false },
    { 0, 0, {}, {}, false },
  };
  struct case_t *c;

  ECHO_OFF();
  for( c=cases; c->n1>0; c++ )
    check_vec_match( c->n1, c->n2, c->val1, c->val2, c->expected );
  ECHO_ON();
}

void check_vec_equal(size_t n1, size_t n2, double *val1, double *val2, bool expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n1, val1 );
  v2 = vec_create_array( n2, val2 );
  if( expected )
    ASSERT_TRUE( vec_equal( v1, v2 ) );
  else
    ASSERT_FALSE( vec_equal( v1, v2 ) );
  vec_destroy( v1 );
  vec_destroy( v2 );
}

TEST(test_vec_equal)
{
  struct case_t {
    size_t n1, n2;
    double val1[10];
    double val2[10];
    bool expected;
  } cases[] = {
    { 2, 2, { 1.0, 1.0 }, { 1.0, 1.0 }, true },
    { 2, 2, { 1.0, 1.0 }, { 2.0, 1.0 }, false },
    { 2, 2, { 1.0, 1.0 }, { 1.0, 2.0 }, false },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, 2.0, 3.0 }, true },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 3.0, 2.0, 1.0 }, false },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, sqrt(2)*sqrt(2), 3.0 }, true },
    { 2, 4, { 1.0, 2.0 }, { 1.0, 2.0, 3.0, 4.0 }, false },
    { 0, 0, {}, {}, false },
  };
  struct case_t *c;

  ECHO_OFF();
  for( c=cases; c->n1>0; c++ )
    check_vec_equal( c->n1, c->n2, c->val1, c->val2, c->expected );
  ECHO_ON();
}

void check_vec_near(size_t n1, size_t n2, double *val1, double *val2, double tol, bool expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n1, val1 );
  v2 = vec_create_array( n2, val2 );
  if( expected )
    ASSERT_TRUE( vec_near( v1, v2, tol ) );
  else
    ASSERT_FALSE( vec_near( v1, v2, tol ) );
  vec_destroy( v1 );
  vec_destroy( v2 );
}

TEST(test_vec_near)
{
  struct case_t {
    size_t n1, n2;
    double val1[10];
    double val2[10];
    double tol;
    bool expected;
  } cases[] = {
    { 2, 2, { 1.0, 1.0 }, { 1.0, 1.0 }, 1e-4, true },
    { 2, 2, { 1.0, 1.0 }, { 2.0, 1.0 }, 1e-4, false },
    { 2, 2, { 1.0, 1.0 }, { 1.0, 1.0+1e-4 }, 1e-6, false },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, 2.0, 3.0 }, 1e-4, true },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, sqrt(2)*sqrt(2), 3.0 }, 1e-25, false },
    { 3, 3, { 1.0, 2.0, 3.0 }, { 1.0, sqrt(2)*sqrt(2), 3.0 }, 1e-6, true },
    { 2, 4, { 1.0, 2.0 }, { 1.0, 2.0, 3.0, 4.0 }, 1e-4, false },
    { 0, 0, {}, {}, false },
  };
  struct case_t *c;

  ECHO_OFF();
  for( c=cases; c->n1>0; c++ )
    check_vec_near( c->n1, c->n2, c->val1, c->val2, c->tol, c->expected );
  ECHO_ON();
}

void check_vec_dot(int n, double *val1, double *val2, double expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n, val1 );
  v2 = vec_create_array( n, val2 );
  ASSERT_DOUBLE_EQ( expected, vec_dot( v1, v2 ) );
  vec_destroy( v1 );
  vec_destroy( v2 );
}

TEST(test_vec_dot)
{
  struct case_t {
    size_t n;
    double val1[10];
    double val2[10];
    double expected;
  } cases[] = {
    { 2, { 0, 0 }, { 0, 0 }, 0 },
    { 2, { 1, 0 }, { 0, 1 }, 0 },
    { 2, { 1, 0 }, { 1, 0 }, 1 },
    { 3, { 1, 2, 1 }, { 2, 0, 1 }, 3 },
    { 3, { -1, 3, -1 }, { 1, -1, 1 }, -5 },
    { 0, {}, {}, 0 },
  };
  struct case_t *c;

  for( c=cases; c->n>0; c++ )
    check_vec_dot( c->n, c->val1, c->val2, c->expected );
}

TEST(test_vec_dot_size_mismatch)
{
  vec_t v1 = vec_create_list( 2, 1, 1 );
  vec_t v2 = vec_create_list( 3, 1, 1, 1 );
  double ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  ret = vec_dot( v1, v2 );
  ASSERT_STREQ( ERR_SIZMIS, __err_last_msg );
  ASSERT_EQ( 0, ret );
  vec_destroy( v1 );
  vec_destroy( v2 );
  ECHO_ON();
}

void check_vec_sqr_norm(size_t n, double *val, double expected)
{
  vec_t v;

  v = vec_create_array( n, val );
  ASSERT_DOUBLE_EQ( expected, vec_sqr_norm( v ) );
  vec_destroy( v );
}

TEST(test_vec_sqr_norm)
{
  struct case_t {
    size_t n;
    double val[10];
    double expected;
  } cases[] = {
    { 2, { 0, 0 }, 0 },
    { 2, { 1, 0 }, 1 },
    { 2, { 1, 1 }, 2 },
    { 3, { 2, 1, 0 }, 5 },
    { 4, { 1, -1, 1, -1 }, 4 },
    { 0, {}, 0 },
  };
  struct case_t *c;

  for( c=cases; c->n>0; c++ )
    check_vec_sqr_norm( c->n, c->val, c->expected );
}

void check_vec_norm(size_t n, double *val, double expected)
{
  vec_t v;

  v = vec_create_array( n, val );
  ASSERT_DOUBLE_EQ( expected, vec_norm( v ) );
  vec_destroy( v );
}

TEST(test_vec_norm)
{
  struct case_t {
    size_t n;
    double val[10];
    double expected;
  } cases[] = {
    { 2, { 0, 0 }, 0 },
    { 2, { 1, 0 }, 1 },
    { 2, { 1, 1 }, sqrt(2) },
    { 3, { 2, 1, 0 }, sqrt(5) },
    { 4, { 1, -1, 1, -1 }, 2 },
    { 0, {}, 0 },
  };
  struct case_t *c;

  for( c=cases; c->n>0; c++ )
    check_vec_norm( c->n, c->val, c->expected );
}

void check_vec_sqr_dist(size_t n, double *val1, double *val2, double expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n, val1 );
  v2 = vec_create_array( n, val2 );
  ASSERT_DOUBLE_EQ( expected, vec_sqr_dist( v1, v2 ) );
  vec_destroy( v1 );
  vec_destroy( v2 );
}

TEST(test_vec_sqr_dist)
{
  struct case_t {
    size_t n;
    double val1[10];
    double val2[10];
    double expected;
  } cases[] = {
    { 2, { 0, 0 }, { 1, 1 }, 2 },
    { 2, { -1, -1 }, { 1, 1 }, 8 },
    { 3, { 1, 2, 3 }, { 4, 5, 6 }, 27 },
    { 3, { 4, 5, 6 }, { 1, 2, 3 }, 27 },
    { 0, {}, {}, 0 },
  };
  struct case_t *c;

  for( c=cases; c->n>0; c++ )
    check_vec_sqr_dist( c->n, c->val1, c->val2, c->expected );
}

TEST(test_vec_sqr_dist_size_mismatch)
{
  vec_t v1 = vec_create_list( 2, 1, 1 );
  vec_t v2 = vec_create_list( 3, 1, 1, 1 );
  double ret;

  RESET_ERR_MSG();
  ECHO_OFF();
  ret = vec_sqr_dist( v1, v2 );
  ASSERT_STREQ( ERR_SIZMIS, __err_last_msg );
  ASSERT_EQ( 0, ret );
  vec_destroy( v1 );
  vec_destroy( v2 );
  ECHO_ON();
}

void check_vec_dist(size_t n, double *val1, double *val2, double expected)
{
  vec_t v1, v2;

  v1 = vec_create_array( n, val1 );
  v2 = vec_create_array( n, val2 );
  ASSERT_DOUBLE_EQ( expected, vec_dist( v1, v2 ) );
  vec_destroy( v1 );
  vec_destroy( v2 );
}

TEST(test_vec_dist)
{
  struct case_t {
    size_t n;
    double val1[10];
    double val2[10];
    double expected;
  } cases[] = {
    { 2, { 0, 0 }, { 1, 1 }, sqrt(2) },
    { 2, { -1, -1 }, { 1, 1 }, 2*sqrt(2) },
    { 3, { 1, 2, 3 }, { 4, 5, 6 }, 3*sqrt(3) },
    { 3, { 4, 5, 6 }, { 1, 2, 3 }, 3*sqrt(3) },
    { 0, {}, {}, 0 },
  };
  struct case_t *c;

  for( c=cases; c->n>0; c++ )
    check_vec_dist( c->n, c->val1, c->val2, c->expected );
}

TEST(test_vec_cos_sim)
{
  struct case_t{
    size_t n;
    double val1[10];
    double val2[10];
    double expected;
  } cases[] = {
    { 2, { 0, 0 }, { 1, 1 }, 0 },
    { 2, { 1, 1 }, { 1, 1 }, 1 },
    { 2, { 1, 1 }, { 2, 2 }, 1 },
    { 2, { 1, 0 }, { 0, 1 }, 0 },
    { 2, { 1, 0 }, { -1, 0 }, -1 },
    { 3, { 1, 2, 1 }, { 2, 1, 2 }, sqrt(6)/3 },
    { 3, { -1, 1, 2 }, { 2, 1, -1 }, -0.5},
    { 3, { -1, 1, 2 }, { 2, -1, -1 }, -5.0/6},
    { 0, {}, {}, 0 },
  };
  struct case_t *c;
  vec_t v1, v2;

  for( c=cases; c->n>0; c++ ){
    v1 = vec_create_array( c->n, c->val1 );
    v2 = vec_create_array( c->n, c->val2 );
    ASSERT_DOUBLE_EQ( c->expected, vec_cos_sim( v1, v2 ) );
    vec_destroy( v1 );
    vec_destroy( v2 );
  }
}

void check_vec_f_write(size_t n, double *val)
{
  vec_t v;
  char expected[BUFSIZ], actual[BUFSIZ], str[BUFSIZ];
  FILE *fp;
  size_t i;

  /* given */
  v = vec_create_array( n, val );
  expected[0] = '\0';
  for( i=0; i<n; i++ ){
    sprintf( str, "%.10g ", vec_elem(v,i) );
    strcat( expected, str );
  }
  strcat( expected, "\n" );
  fp = tmpfile();
  /* when */
  vec_f_write( fp, v );
  /* then */
  rewind( fp );
  if( fgets( actual, BUFSIZ, fp ) == NULL )
    FAIL( "failure on 'fgets'" );
  ASSERT_STREQ( expected, actual );
  /* clean */
  fclose( fp );
  vec_destroy( v );
}

TEST(test_vec_f_write)
{
  struct case_t {
    size_t n;
    double val[10];
  } cases[] = {
    { 3, { 0.0, 0.1, 0.2 } },
    { 5, { 1.0, 2.33, 3.44, -2.1, -5.3 } },
    { 0, {} }
  };
  struct case_t *c;

  for( c=cases; c->n>0; c++ )
    check_vec_f_write( c->n, c->val );
}

TEST(test_vec_f_write_given_null)
{
  FILE *fp = tmpfile();

  vec_f_write( fp, NULL );
  fclose( fp );
}

TEST_SUITE(test_vec)
{
  RUN_TEST(test_vec_create);
  RUN_TEST(test_vec_create_zero_size);
  RUN_TEST(test_destroy);
  RUN_TEST(test_destroy_null);
  RUN_TEST(test_vec_set_elem);
  RUN_TEST(test_vec_set_elem_list);
  RUN_TEST(test_vec_set_elem_array);
  RUN_TEST(test_vec_create_list);
  RUN_TEST(test_vec_create_array);
  RUN_TEST(test_vec_fill);
  RUN_TEST(test_vec_clear);
  RUN_TEST(test_vec_add);
  RUN_TEST(test_vec_add_size_mismatch);
  RUN_TEST(test_vec_sub);
  RUN_TEST(test_vec_sub_size_mismatch);
  RUN_TEST(test_vec_mul);
  RUN_TEST(test_vec_mul_size_mismatch);
  RUN_TEST(test_vec_div);
  RUN_TEST(test_vec_div_size_mismatch);
  RUN_TEST(test_vec_div_by_zero);
  RUN_TEST(test_vec_cat);
  RUN_TEST(test_vec_cat_size_mismatch);
  RUN_TEST(test_vec_copy);
  RUN_TEST(test_vec_copy_size_mismatch);
  RUN_TEST(test_vec_clone);
  RUN_TEST(test_vec_match);
  RUN_TEST(test_vec_equal);
  RUN_TEST(test_vec_near);
  RUN_TEST(test_vec_dot);
  RUN_TEST(test_vec_dot_size_mismatch);
  RUN_TEST(test_vec_sqr_norm);
  RUN_TEST(test_vec_norm);
  RUN_TEST(test_vec_sqr_dist);
  RUN_TEST(test_vec_sqr_dist_size_mismatch);
  RUN_TEST(test_vec_dist);
  RUN_TEST(test_vec_cos_sim);
  RUN_TEST(test_vec_f_write);
  RUN_TEST(test_vec_f_write_given_null);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_vec);
  TEST_REPORT();
  TEST_EXIT();
}
