#include "rhc_ctrl.h"

ctrl_t *ctrl_init(ctrl_t *self, cmd_t *cmd)
{
  self->cmd = cmd;
  self->fz = 0;
  self->_update = ctrl_update_default;
  self->_destroy = ctrl_destroy_default;
  self->prp = NULL;
  return self;
}

void ctrl_destroy_default(ctrl_t *self)
{
  self->cmd = NULL;
  self->prp = NULL;
}

bool ctrl_is_in_flight(ctrl_t *self, vec_t p)
{
  return ( vec_elem(p,0) > ctrl_z0(self) );
}

bool ctrl_is_in_compression(ctrl_t *self, vec_t p)
{
  return ( vec_elem(p,1) < 0 ) && ( vec_elem(p,0) <= ctrl_z0(self) );
}

bool ctrl_is_in_decompression(ctrl_t *self, vec_t p)
{
  return ( vec_elem(p,1) >= 0 ) && ( vec_elem(p,0) <= ctrl_z0(self) );
}

double ctrl_calc_sqr_v0(double z0, double zd)
{
  return 2.0 * G * ( zd - z0 );
}

ctrl_t *ctrl_update_default(ctrl_t *self, double t, vec_t p)
{
  /* dummy */
  return self;
}

ctrl_t *ctrl_sub_create(ctrl_t *self, cmd_t *cmd, double k)
{
  _ctrl_sub_prp *prp;

  ctrl_init( self, cmd  );
  self->_update = ctrl_sub_update;
  self->_destroy = ctrl_sub_destroy;

  if( ( self->prp = nalloc( _ctrl_sub_prp, 1 ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  prp = self->prp;
  prp->k = k;
  return self;
}

void ctrl_sub_destroy(ctrl_t *self)
{
  sfree( self->prp );
  ctrl_destroy_default( self );
}

ctrl_t *ctrl_sub_update(ctrl_t *self, double t, vec_t p)
{
  _ctrl_sub_prp *prp;

  prp = self->prp;
  self->fz = prp->k;
  return self;
}
