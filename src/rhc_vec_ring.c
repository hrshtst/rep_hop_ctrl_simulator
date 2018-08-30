#include "rhc_vec_ring.h"

void vec_ring_init(vec_ring_t *self, int size)
{
  register int i;

  self->head = -1;
  self->max = size;
  if( ( vec_ring_buffer(self) = nalloc( vec_t, size ) ) == NULL ){
    ALLOC_ERR();
    return;
  }
  for( i=0; i<size; i++ ){
    vec_ring_buffer(self)[i] = vec_create( 2 );
  }
}

void vec_ring_destroy(vec_ring_t *self)
{}

void vec_ring_push(vec_ring_t *self, vec_t v)
{
  self->head++;
  vec_copy( v, vec_ring_head(self) );
}

void vec_ring_pop(vec_ring_t *self)
{
  self->head--;
}

bool vec_ring_empty(vec_ring_t *self)
{
  return ( self->head == 0 );
}

bool vec_ring_full(vec_ring_t *self)
{
  return ( vec_ring_size(self) == vec_ring_capacity(self) );
}
