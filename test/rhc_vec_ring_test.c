#include "rhc_vec_ring.h"
#include "rhc_test.h"

#define SIZE 3
#define DIM  2
vec_ring_t ring;
vec_t v[SIZE];

double randf(double min, double max)
{
  return (double)rand() / RAND_MAX * ( max - min ) + min;
}

void setup()
{
  register int i, j;

  vec_ring_init( &ring, DIM, SIZE );
  for( i=0; i<SIZE; i++ )
    v[i] = vec_create( DIM );

  srand( (int)time(NULL) );
  for( i=0; i<SIZE; i++ ){
    for( j=0; j<DIM; j++ ){
      vec_set_elem( v[i], j, randf( -10, 10 ) );
    }
  }
}

void teardown()
{
  register int i;

  vec_ring_destroy( &ring );
  for( i=0; i<SIZE; i++ )
    vec_destroy( v[i] );
}

TEST(test_vec_ring_init)
{
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
  ASSERT_EQ( 0, vec_ring_size(&ring) );
  ASSERT_EQ( DIM, vec_ring_dim(&ring) );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
  if( SIZE > 0 )
    ASSERT_EQ( DIM, vec_size( vec_ring_head(&ring) ) );
}

TEST(test_vec_ring_init2)
{
  vec_ring_t vr;
  const int dim = 4;
  const int size = 6;

  vec_ring_init( &vr, dim, size );
  ASSERT_EQ( 0, vec_ring_head_index(&vr) );
  ASSERT_EQ( 0, vec_ring_size(&vr) );
  ASSERT_EQ( dim, vec_ring_dim(&vr) );
  ASSERT_EQ( size, vec_ring_capacity(&vr) );
  if( size > 0 )
    ASSERT_EQ( dim, vec_size( vec_ring_head(&vr) ) );
  vec_ring_destroy( &vr );
}

TEST(test_vec_ring_push_one)
{
  register int i;

  /* push one vector */
  vec_ring_push( &ring, v[0] );
  for( i=0; i<DIM; i++ ){
    ASSERT_EQ( vec_elem( v[0], i ), vec_elem( vec_ring_head(&ring), i ) );
  }
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
  ASSERT_EQ( 1, vec_ring_size(&ring) );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );
}

TEST(test_vec_ring_capacity)
{
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );

  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );
  vec_ring_push( &ring, v[1] );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );
  vec_ring_push( &ring, v[2] );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );

  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_capacity(&ring) );
}

TEST(test_vec_ring_size)
{
  ASSERT_EQ( 0, vec_ring_size(&ring) );

  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( 1, vec_ring_size(&ring) );
  vec_ring_push( &ring, v[1] );
  ASSERT_EQ( 2, vec_ring_size(&ring) );
  vec_ring_push( &ring, v[2] );
  ASSERT_EQ( 3, vec_ring_size(&ring) );

  vec_ring_pop( &ring );
  ASSERT_EQ( 2, vec_ring_size(&ring) );
  vec_ring_pop( &ring );
  ASSERT_EQ( 1, vec_ring_size(&ring) );
  vec_ring_pop( &ring );
  ASSERT_EQ( 0, vec_ring_size(&ring) );
}

TEST(test_vec_ring_empty)
{
  ASSERT_TRUE( vec_ring_empty(&ring) );

  vec_ring_push( &ring, v[0] );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_push( &ring, v[1] );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_push( &ring, v[2] );
  ASSERT_FALSE( vec_ring_empty(&ring) );

  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_pop( &ring );
  ASSERT_TRUE( vec_ring_empty(&ring) );
}

TEST(test_vec_ring_empty2)
{
  ASSERT_TRUE( vec_ring_empty(&ring) );

  vec_ring_push( &ring, v[0] );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_push( &ring, v[1] );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_push( &ring, v[2] );
  ASSERT_FALSE( vec_ring_empty(&ring) );

  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_empty(&ring) );
  vec_ring_pop( &ring );
  ASSERT_TRUE( vec_ring_empty(&ring) );
}

TEST(test_vec_ring_full)
{
  ASSERT_FALSE( vec_ring_full(&ring) );

  vec_ring_push( &ring, v[0] );
  ASSERT_FALSE( vec_ring_full(&ring) );
  vec_ring_push( &ring, v[1] );
  ASSERT_FALSE( vec_ring_full(&ring) );
  vec_ring_push( &ring, v[2] );
  ASSERT_TRUE( vec_ring_full(&ring) );

  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_full(&ring) );
  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_full(&ring) );
  vec_ring_pop( &ring );
  ASSERT_FALSE( vec_ring_full(&ring) );
}

TEST(test_vec_ring_push)
{
  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( vec_elem( v[0], 0 ), vec_elem( vec_ring_head(&ring), 0 ) );
}

TEST_SUITE(test_vec_ring)
{
  CONFIGURE_SUITE(setup, teardown);
  RUN_TEST(test_vec_ring_init);
  RUN_TEST(test_vec_ring_init2);
  RUN_TEST(test_vec_ring_push_one);
  /* RUN_TEST(test_vec_ring_capacity); */
  /* RUN_TEST(test_vec_ring_size); */
  /* RUN_TEST(test_vec_ring_empty); */
  /* RUN_TEST(test_vec_ring_empty2); */
  /* RUN_TEST(test_vec_ring_full); */
  /* RUN_TEST(test_vec_ring_push); */
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_vec_ring);
  TEST_REPORT();
  TEST_EXIT();
}
