#include "rhc_vec_list.h"

vec_list_node_t *vec_list_node_init(vec_list_node_t *self)
{
  self->v = NULL;
  self->prev = self;
  self->next = self;
  return self;
}

void vec_list_node_destroy(vec_list_node_t *self)
{
  vec_destroy( vec_list_node_data(self) );
  self->v = NULL;
  self->prev = NULL;
  self->next = NULL;
}

vec_list_node_t *vec_list_node_insert_prev(vec_list_node_t *self, vec_list_node_t *node_new)
{
  node_new->next = self;
  self->prev->next = node_new;
  node_new->prev = self->prev;
  self->prev = node_new;
  return node_new;
}

vec_list_node_t *vec_list_node_insert_next(vec_list_node_t *self, vec_list_node_t *node_new)
{
  node_new->prev = self;
  self->next->prev = node_new;
  node_new->next = self->next;
  self->next = node_new;
  return node_new;
}

vec_list_node_t *vec_list_node_delete_prev(vec_list_node_t *self)
{
  vec_list_node_t *node;

  node = self->prev;
  node->prev->next = self;
  self->prev = node->prev;
  node->prev = node;
  node->next = node;
  return node;
}

vec_list_node_t *vec_list_node_delete_next(vec_list_node_t *self)
{
  vec_list_node_t *node;

  node = self->next;
  node->next->prev = self;
  self->next = node->next;
  node->next = node;
  node->prev = node;
  return node;
}

vec_list_node_t *vec_list_node_delete(vec_list_node_t *self)
{
  return vec_list_node_delete_next( vec_list_node_prev(self) );
}

vec_t vec_list_node_set_data(vec_list_node_t *self, vec_t v)
{
  self->v = v;
  return v;
}

vec_list_t *vec_list_init(vec_list_t *self)
{
  vec_list_node_init( vec_list_root(self) );
  self->num = 0;
  return self;
}

void vec_list_destroy(vec_list_t *self)
{
  vec_list_node_t *node;

  while( !vec_list_is_empty(self) ){
    node = vec_list_delete_tail( self );
    vec_list_node_destroy( node );
    sfree( node );
  }
}

vec_list_node_t *vec_list_insert_prev(vec_list_t *self, vec_list_node_t *node, vec_list_node_t *node_new)
{
  vec_list_inc( self );
  return vec_list_node_insert_prev( node, node_new );
}

vec_list_node_t *vec_list_insert_next(vec_list_t *self, vec_list_node_t *node, vec_list_node_t *node_new)
{
  vec_list_inc( self );
  return vec_list_node_insert_next( node, node_new );
}

vec_list_node_t *vec_list_delete_prev(vec_list_t *self, vec_list_node_t *node)
{
  vec_list_dec( self );
  return vec_list_node_delete_prev( node );
}

vec_list_node_t *vec_list_delete_next(vec_list_t *self, vec_list_node_t *node)
{
  vec_list_dec( self );
  return vec_list_node_delete_next( node );
}

vec_list_node_t *vec_list_delete(vec_list_t *self, vec_list_node_t *node)
{
  vec_list_dec( self );
  return vec_list_node_delete( node );
}
