#include "rhc_vec_list.h"

vec_list_node_t *vec_list_node_init(vec_list_node_t *self)
{
  self->v = NULL;
  self->next = self;
  return self;
}

void vec_list_node_destroy(vec_list_node_t *self)
{
  vec_destroy( vec_list_node_data(self) );
  self->v = NULL;
  self->next = NULL;
}

vec_list_node_t *vec_list_node_insert_next(vec_list_node_t *self, vec_list_node_t *node)
{
  self->next = node;
  return node;
}

vec_list_node_t *vec_list_node_delete_next(vec_list_node_t *self)
{
  vec_list_node_t *node;

  node = self->next;
  self->next = self;
  return node;
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
}
