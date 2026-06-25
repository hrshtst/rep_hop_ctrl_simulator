#include "rhc_vec_ring.h"

static int vec_ring_head_reset(vec_ring_t *self);
static int vec_ring_head_advance(vec_ring_t *self);
static int vec_ring_size_reset(vec_ring_t *self);
static int vec_ring_size_increase(vec_ring_t *self);
static int vec_ring_size_decrease(vec_ring_t *self);

int vec_ring_head_reset(vec_ring_t *self)
{
  self->head = 0;
  return vec_ring_head_index(self);
}

int vec_ring_head_advance(vec_ring_t *self)
{
  self->head++;
  self->head = vec_ring_index( self, self->head );
  return vec_ring_head_index(self);
}

int vec_ring_size_reset(vec_ring_t *self)
{
  self->size = 0;
  return vec_ring_size(self);
}

int vec_ring_size_increase(vec_ring_t *self)
{
  if( !vec_ring_full(self) )
    self->size++;
  return vec_ring_size(self);
}

int vec_ring_size_decrease(vec_ring_t *self)
{
  if( !vec_ring_empty(self) )
    self->size--;
  return vec_ring_size(self);
}


void vec_ring_init(vec_ring_t *self, int dim, int size)
{
  int i;

  vec_ring_head_reset( self );
  vec_ring_size_reset( self );
  self->dim = dim;
  self->max = size;
  if( ( vec_ring_buf(self) = nalloc( vec_t, size ) ) == NULL ){
    ALLOC_ERR();
    return;
  }
  for( i=0; i<vec_ring_capacity(self); i++ ){
    vec_ring_buf(self)[i] = vec_create( vec_ring_dim(self) );
  }
}

void vec_ring_destroy(vec_ring_t *self)
{
  int i;

  if( !vec_ring_buf(self) ) return;
  for( i=0; i<vec_ring_capacity(self); i++ ){
    vec_destroy( vec_ring_buf(self)[i] );
  }
  sfree( vec_ring_buf(self) );
}

void vec_ring_push(vec_ring_t *self, vec_t v)
{
  if( !vec_ring_empty(self) )
    vec_ring_head_advance( self );
  vec_ring_size_increase( self );
  vec_copy( v, vec_ring_head(self) );
}

vec_t vec_ring_pop(vec_ring_t *self)
{
  vec_t ret;

  if( vec_ring_empty(self) )
    return NULL;
  ret = vec_ring_tail(self);
  vec_ring_size_decrease( self );
  return ret;
}

bool vec_ring_empty(vec_ring_t *self)
{
  return ( vec_ring_size(self) == 0 );
}

bool vec_ring_full(vec_ring_t *self)
{
  return ( vec_ring_size(self) == vec_ring_capacity(self) );
}

void vec_ring_reset(vec_ring_t *self)
{
  vec_ring_size_reset( self );
}
