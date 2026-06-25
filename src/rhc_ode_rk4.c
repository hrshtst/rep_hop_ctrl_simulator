#include "rhc_ode.h"

typedef struct{
  vec_t x, k[4];
} _ode_rk4;

ode_t *ode_init_rk4(ode_t *self, int dim, vec_t (* f)(double,vec_t,void*,vec_t))
{
  _ode_rk4 *ws;

  if( ( ws = nalloc( _ode_rk4, 1 ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  /* ws is calloc'd, so any vec not yet created is NULL and safe to pass
     to vec_destroy() during cleanup. */
  if( !( ws->x = vec_create( dim ) )    ||
      !( ws->k[0] = vec_create( dim ) ) ||
      !( ws->k[1] = vec_create( dim ) ) ||
      !( ws->k[2] = vec_create( dim ) ) ||
      !( ws->k[3] = vec_create( dim ) ) ){
    ALLOC_ERR();
    vec_destroy( ws->x );
    vec_destroy( ws->k[0] );
    vec_destroy( ws->k[1] );
    vec_destroy( ws->k[2] );
    vec_destroy( ws->k[3] );
    free( ws );
    return NULL;
  }
  self->f = f;
  self->_ws = ws;
  return self;
}

vec_t ode_update_rk4(ode_t *self, double t, vec_t x, double dt, void *util)
{
  _ode_rk4 *ws;
  double dt1, dt2, dt3;

  ws = self->_ws;
  dt1 = dt * 0.5;
  dt2 = dt / 6;
  dt3 = dt2 * 2;
  self->f( t, x, util, ws->k[0] );
  vec_cat( x, dt1, ws->k[0], ws->x );
  self->f( t+dt1, ws->x, util, ws->k[1] );
  vec_cat( x, dt1, ws->k[1], ws->x );
  self->f( t+dt1, ws->x, util, ws->k[2] );
  vec_cat( x, dt, ws->k[2], ws->x );
  self->f( t+dt, ws->x, util, ws->k[3] );

  vec_cat( x, dt2, ws->k[0], x );
  vec_cat( x, dt3, ws->k[1], x );
  vec_cat( x, dt3, ws->k[2], x );
  vec_cat( x, dt2, ws->k[3], x );
  return x;
}

void ode_destroy_rk4(ode_t *self)
{
  _ode_rk4 *ws;

  ws = self->_ws;
  vec_destroy( ws->x );
  vec_destroy( ws->k[0] );
  vec_destroy( ws->k[1] );
  vec_destroy( ws->k[2] );
  vec_destroy( ws->k[3] );
  sfree( self->_ws );
  self->f = NULL;
}
