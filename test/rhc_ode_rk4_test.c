#include "rhc_ode.h"
#include "rhc_test.h"

/* unit circle function */
vec_t dp(double t, vec_t p, void *dummy, vec_t v)
{
  vec_set_elem( v, 0, -vec_elem(p,1) );
  vec_set_elem( v, 1,  vec_elem(p,0) );
  return v;
}

double unit_circle(vec_t x)
{
  return sqr( vec_elem(x,0) ) + sqr( vec_elem(x,1) ) - 1.0;
}

bool curve_fit(vec_t x, double (*f)(vec_t), double tol)
{
  return istol( f(x), tol ) ? true : false;
}

ode_t ode;
vec_t x;
double t;
const double T  = 10;
const double DT = 0.01;

void setup()
{
  ode_assign( &ode, rk4 );
  ode_init( &ode, 2, dp );
  x = vec_create_list( 2, 1.0, 0.0 );
}

void teardown()
{
  vec_destroy( x );
  if( ode._ws )
    ode_destroy( &ode );
}


TEST(test_ode_rk4_init)
{
  ASSERT_PTREQ( dp, ode.f );
  ASSERT_PTRNE( NULL, ode._ws );
}

TEST(test_ode_rk4_destroy)
{
  ode_destroy( &ode );
  ASSERT_PTREQ( NULL, ode.f );
  ASSERT_PTREQ( NULL, ode._ws );
}

TEST(test_ode_rk4_update)
{
  ode_update( &ode, t, x, DT, NULL );
  ASSERT_NEAR( 0.99995,      vec_elem(x,0), 1e-6 );
  ASSERT_NEAR( 0.0099998333, vec_elem(x,1), 1e-6 );
  ode_update( &ode, t, x, DT, NULL );
  ASSERT_NEAR( 0.9998,       vec_elem(x,0), 1e-6 );
  ASSERT_NEAR( 0.019998666,  vec_elem(x,1), 1e-6 );
}

TEST(test_ode_rk4_integrate)
{
  char msg[RHC_BUFSIZ];

  for( t=0; t<T; t+=DT ){
    ode_update( &ode, t, x, DT, NULL );
    sprintf( msg, "not fit to unit circle: t=%f", t );
    ASSERT( curve_fit(x,unit_circle,1e-10), msg );
  }
}

TEST_SUITE(test_ode_rk4)
{
  CONFIGURE_SUITE( &setup, &teardown );
  RUN_TEST(test_ode_rk4_init);
  RUN_TEST(test_ode_rk4_destroy);
  RUN_TEST(test_ode_rk4_update);
  RUN_TEST(test_ode_rk4_integrate);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_ode_rk4);
  TEST_REPORT();
  TEST_EXIT();
}
