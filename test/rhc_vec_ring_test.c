#include "rhc_test.h"
#include "rhc_vec_ring.h"

#define SIZE 7
#define NUM_VEC ( SIZE + 3 )
#define DIM  3
vec_ring_t ring;
vec_t v[NUM_VEC];

double randf(double min, double max)
{
  return (double)rand() / RAND_MAX * ( max - min ) + min;
}

void assert_vec(vec_t v1, vec_t v2)
{
  size_t i;

  ASSERT_EQ( vec_size(v1), vec_size(v2) );
  for( i=0; i< vec_size(v1); i++ )
    ASSERT_DOUBLE_EQ( vec_elem( v1, i ), vec_elem( v2, i ) );
}

void setup()
{
  int i, j;

  vec_ring_init( &ring, DIM, SIZE );
  for( i=0; i<NUM_VEC; i++ )
    v[i] = vec_create( DIM );

  srand( (int)time(NULL) );
  for( i=0; i<NUM_VEC; i++ ){
    for( j=0; j<DIM; j++ ){
      vec_set_elem( v[i], j, randf( -10, 10 ) );
    }
  }
}

void teardown()
{
  int i;

  vec_ring_destroy( &ring );
  for( i=0; i<NUM_VEC; i++ )
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

TEST(test_vec_ring_capacity)
{
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );

  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
  vec_ring_push( &ring, v[1] );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
  vec_ring_push( &ring, v[2] );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );

  vec_ring_pop( &ring );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
  vec_ring_pop( &ring );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
  vec_ring_pop( &ring );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
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
  int i;

  ASSERT_TRUE( vec_ring_empty(&ring) );

  for( i=0; i<SIZE; i++ ){
    vec_ring_push( &ring, v[i] );
    ASSERT_FALSE( vec_ring_empty(&ring) );
  }

  for( i=0; i<SIZE-1; i++ ){
    vec_ring_pop( &ring );
    ASSERT_FALSE( vec_ring_empty(&ring) );
  }
  vec_ring_pop( &ring );
  ASSERT_TRUE( vec_ring_empty(&ring) );
}

TEST(test_vec_ring_full)
{
  int i;

  ASSERT_FALSE( vec_ring_full(&ring) );

  for( i=0; i<SIZE-1; i++ ){
    vec_ring_push( &ring, v[i] );
    ASSERT_FALSE( vec_ring_full(&ring) );
  }
  vec_ring_push( &ring, v[SIZE-1] );
  ASSERT_TRUE( vec_ring_full(&ring) );

  for( i=0; i<SIZE; i++ ){
    vec_ring_pop( &ring );
    ASSERT_FALSE( vec_ring_full(&ring) );
  }
}

TEST(test_vec_ring_head_index)
{
  /* push 4 vectors */
  vec_ring_push( &ring, v[0] );  /* <-tail */
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  vec_ring_push( &ring, v[3] );  /* <-head */

  /* check head */
  ASSERT_EQ( 3, vec_ring_head_index(&ring) );
  /* pop one then check head */
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_head_index(&ring) );
  /* pop one then check head */
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_head_index(&ring) );
}

TEST(test_vec_ring_tail_index)
{
  /* push 4 vectors */
  vec_ring_push( &ring, v[0] );  /* <-tail */
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  vec_ring_push( &ring, v[3] );  /* <-head */

  /* check tail */
  ASSERT_EQ( 0, vec_ring_tail_index(&ring) );
  /* pop one then check tail */
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_head_index(&ring) );
  ASSERT_EQ( 3, vec_ring_size(&ring) );
  ASSERT_EQ( 1, vec_ring_tail_index(&ring) );
}

TEST(test_vec_ring_item_index)
{
  /* push 4 vectors */
  vec_ring_push( &ring, v[0] );  /* <-tail */
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  vec_ring_push( &ring, v[3] );  /* <-head */

  /* check item */
  ASSERT_EQ( 0, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 1, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 2) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 3) );
  /* pop one then check item */
  vec_ring_pop( &ring );
  ASSERT_EQ( 1, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 2) );
  /* pop one then check item */
  vec_ring_pop( &ring );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 1) );
  /* pop one then check item */
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 0) );
}

void print_index(vec_ring_t *vr, int line)
{
  fprintf( stderr, "tail: %d ---- head: %d <size: %d> (LINE: %d)\n",
           vec_ring_tail_index(&ring), vec_ring_head_index(&ring),
           vec_ring_size(&ring), line );
}

TEST(test_vec_ring_head_and_tail_indices)
{
  /* max size is set to 7 */
  ASSERT_EQ( 7, vec_ring_capacity(&ring) );

  /* push 5 vectors */
  vec_ring_push( &ring, v[0] );
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  vec_ring_push( &ring, v[3] );
  vec_ring_push( &ring, v[4] );
  ASSERT_EQ( 5, vec_ring_size(&ring) );
  ASSERT_EQ( 4, vec_ring_head_index(&ring) );
  ASSERT_EQ( 0, vec_ring_tail_index(&ring) );

  /* pop 2 vectors */
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  ASSERT_EQ( 3, vec_ring_size(&ring) );
  ASSERT_EQ( 4, vec_ring_head_index(&ring) );
  ASSERT_EQ( 2, vec_ring_tail_index(&ring) );

  /* push 4 vectors */
  vec_ring_push( &ring, v[5] );
  vec_ring_push( &ring, v[6] );
  vec_ring_push( &ring, v[7] );
  vec_ring_push( &ring, v[8] );
  ASSERT_EQ( 7, vec_ring_size(&ring) );
  ASSERT_EQ( 1, vec_ring_head_index(&ring) );
  ASSERT_EQ( 2, vec_ring_tail_index(&ring) );

  /* push more 2 vectors */
  vec_ring_push( &ring, v[9] );
  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( 7, vec_ring_size(&ring) );
  ASSERT_EQ( 3, vec_ring_head_index(&ring) );
  ASSERT_EQ( 4, vec_ring_tail_index(&ring) );

  /* pop 3 vectors */
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  ASSERT_EQ( 4, vec_ring_size(&ring) );
  ASSERT_EQ( 3, vec_ring_head_index(&ring) );
  ASSERT_EQ( 0, vec_ring_tail_index(&ring) );
}

TEST(test_vec_ring_item_index2)
{
  /* max size is set to 7 */
  ASSERT_EQ( 7, vec_ring_capacity(&ring) );

  /* push 5 vectors */
  vec_ring_push( &ring, v[0] );
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  vec_ring_push( &ring, v[3] );
  vec_ring_push( &ring, v[4] );
  ASSERT_EQ( 0, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 1, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 2) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 3) );
  ASSERT_EQ( 4, vec_ring_item_index(&ring, 4) );

  /* pop 2 vectors */
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 4, vec_ring_item_index(&ring, 2) );

  /* push 4 vectors */
  vec_ring_push( &ring, v[5] );
  vec_ring_push( &ring, v[6] );
  vec_ring_push( &ring, v[7] );
  vec_ring_push( &ring, v[8] );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 4, vec_ring_item_index(&ring, 2) );
  ASSERT_EQ( 5, vec_ring_item_index(&ring, 3) );
  ASSERT_EQ( 6, vec_ring_item_index(&ring, 4) );
  ASSERT_EQ( 0, vec_ring_item_index(&ring, 5) );
  ASSERT_EQ( 1, vec_ring_item_index(&ring, 6) );

  /* push more 2 vectors */
  vec_ring_push( &ring, v[9] );
  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( 4, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 5, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 6, vec_ring_item_index(&ring, 2) );
  ASSERT_EQ( 0, vec_ring_item_index(&ring, 3) );
  ASSERT_EQ( 1, vec_ring_item_index(&ring, 4) );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 5) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 6) );

  /* pop 3 vectors */
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  ASSERT_EQ( 0, vec_ring_item_index(&ring, 0) );
  ASSERT_EQ( 1, vec_ring_item_index(&ring, 1) );
  ASSERT_EQ( 2, vec_ring_item_index(&ring, 2) );
  ASSERT_EQ( 3, vec_ring_item_index(&ring, 3) );
}

TEST(test_vec_ring_push_one)
{
  vec_ring_push( &ring, v[0] );
  assert_vec( v[0], vec_ring_head(&ring) );
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
  ASSERT_EQ( 1, vec_ring_size(&ring) );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
}

TEST(test_vec_ring_pop_nothing)
{
  /* if size of ring buffer equals to 0 */
  ASSERT_EQ( 0, vec_ring_size(&ring) );
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
  /* then pop returns NULL pointer */
  ASSERT_PTREQ( NULL, vec_ring_pop(&ring) );
  ASSERT_EQ( 0, vec_ring_size(&ring) );
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
}

TEST(test_vec_ring_pop_one)
{
  vec_t tmp;

  vec_ring_push( &ring, v[0] );
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
  ASSERT_EQ( 1, vec_ring_size(&ring) );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );

  tmp = vec_ring_pop( &ring );
  assert_vec( v[0], tmp );
  ASSERT_EQ( 0, vec_ring_head_index(&ring) );
  ASSERT_EQ( 0, vec_ring_size(&ring) );
  ASSERT_EQ( SIZE, vec_ring_capacity(&ring) );
}

TEST(test_vec_ring_push_until_full)
{
  int i;

  ASSERT_TRUE( vec_ring_empty(&ring) );
  for( i=0; i<SIZE; i++ ){
    vec_ring_push( &ring, v[i] );
    ASSERT_EQ( i+1, vec_ring_size(&ring) );
    assert_vec( v[i], vec_ring_head(&ring) );
  }
  ASSERT_TRUE( vec_ring_full(&ring) );
}

void push_until_full(vec_ring_t *vr)
{
  int i;

  for( i=vec_ring_size(vr); i<vec_ring_capacity(vr); i++ ){
    vec_ring_push( vr, v[i] );
  }
}

TEST(test_vec_ring_push_over_capacity)
{
  push_until_full( &ring );
  ASSERT_TRUE( vec_ring_full(&ring) );

  /* push one vector */
  vec_ring_push( &ring, v[SIZE] );
  ASSERT_EQ( SIZE, vec_ring_size(&ring) );
  assert_vec( v[SIZE], vec_ring_head(&ring) );
  ASSERT_TRUE( vec_ring_full(&ring) );
  /* push one more vector */
  vec_ring_push( &ring, v[SIZE+1] );
  ASSERT_EQ( SIZE, vec_ring_size(&ring) );
  assert_vec( v[SIZE+1], vec_ring_head(&ring) );
  ASSERT_TRUE( vec_ring_full(&ring) );
}

TEST(test_vec_ring_push_some_then_pop_them)
{
  vec_t tmp;

  /* push 3 vectors */
  vec_ring_push( &ring, v[0] );
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  ASSERT_EQ( 3, vec_ring_size(&ring) );

  /* pop the first vector */
  tmp = vec_ring_pop( &ring );
  assert_vec( v[0], tmp );
  ASSERT_EQ( 2, vec_ring_size(&ring) );
  /* pop the second vector */
  tmp = vec_ring_pop( &ring );
  assert_vec( v[1], tmp );
  ASSERT_EQ( 1, vec_ring_size(&ring) );
  /* pop the third vector */
  tmp = vec_ring_pop( &ring );
  assert_vec( v[2], tmp );
  ASSERT_EQ( 0, vec_ring_size(&ring) );
}

TEST(test_vec_ring_push_much_pop_some_then_push_again)
{
  /* push until full and much more */
  push_until_full( &ring );
  vec_ring_push( &ring, v[SIZE] );
  vec_ring_push( &ring, v[SIZE+1] );
  vec_ring_push( &ring, v[SIZE+2] );
  ASSERT_EQ( SIZE, vec_ring_size(&ring) );
  ASSERT_TRUE( vec_ring_full(&ring) );

  /* pop some */
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  ASSERT_EQ( SIZE-4, vec_ring_size(&ring) );
  ASSERT_FALSE( vec_ring_full(&ring) );

  /* push again */
  vec_ring_push( &ring, v[SIZE-1] );
  vec_ring_push( &ring, v[SIZE-2] );
  ASSERT_EQ( SIZE-2, vec_ring_size(&ring) );
  ASSERT_FALSE( vec_ring_full(&ring) );
}

TEST(test_vec_ring_reset)
{
  vec_ring_push( &ring, v[0] );
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );
  vec_ring_push( &ring, v[3] );
  ASSERT_FALSE( vec_ring_empty(&ring) );

  vec_ring_reset( &ring );
  ASSERT_TRUE( vec_ring_empty(&ring) );
  ASSERT_PTREQ( NULL, vec_ring_pop(&ring) );

  push_until_full( &ring );
  ASSERT_TRUE( vec_ring_full(&ring) );

  vec_ring_reset( &ring );
  ASSERT_TRUE( vec_ring_empty(&ring) );
  ASSERT_PTREQ( NULL, vec_ring_pop(&ring) );
}

TEST(test_vec_ring_item)
{
  vec_ring_push( &ring, v[0] );
  vec_ring_push( &ring, v[1] );
  vec_ring_push( &ring, v[2] );

  assert_vec( v[0], vec_ring_item( &ring, 0 ) );
  assert_vec( v[1], vec_ring_item( &ring, 1 ) );
  assert_vec( v[2], vec_ring_item( &ring, 2 ) );

  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  vec_ring_pop( &ring );
  push_until_full( &ring );

  assert_vec( v[0], vec_ring_item( &ring, 0 ) );
  assert_vec( v[1], vec_ring_item( &ring, 1 ) );
  assert_vec( v[2], vec_ring_item( &ring, 2 ) );
  assert_vec( v[3], vec_ring_item( &ring, 3 ) );
  assert_vec( v[4], vec_ring_item( &ring, 4 ) );
  assert_vec( v[5], vec_ring_item( &ring, 5 ) );
  assert_vec( v[6], vec_ring_item( &ring, 6 ) );
}

TEST_SUITE(test_vec_ring)
{
  CONFIGURE_SUITE(setup, teardown);
  RUN_TEST(test_vec_ring_init);
  RUN_TEST(test_vec_ring_init2);
  RUN_TEST(test_vec_ring_capacity);
  RUN_TEST(test_vec_ring_size);
  RUN_TEST(test_vec_ring_empty);
  RUN_TEST(test_vec_ring_full);
  RUN_TEST(test_vec_ring_head_index);
  RUN_TEST(test_vec_ring_tail_index);
  RUN_TEST(test_vec_ring_item_index);
  RUN_TEST(test_vec_ring_head_and_tail_indices);
  RUN_TEST(test_vec_ring_item_index2);
  RUN_TEST(test_vec_ring_push_one);
  RUN_TEST(test_vec_ring_pop_nothing);
  RUN_TEST(test_vec_ring_pop_one);
  RUN_TEST(test_vec_ring_push_until_full);
  RUN_TEST(test_vec_ring_push_over_capacity);
  RUN_TEST(test_vec_ring_push_some_then_pop_them);
  RUN_TEST(test_vec_ring_push_much_pop_some_then_push_again);
  RUN_TEST(test_vec_ring_reset);
  RUN_TEST(test_vec_ring_item);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_vec_ring);
  TEST_REPORT();
  TEST_EXIT();
}
