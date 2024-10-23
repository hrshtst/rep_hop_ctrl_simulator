#include "rhc_ctrl_arl.h"
#include "rhc_ctrl.h"

ctrl_t *ctrl_arl_create(ctrl_t *self, cmd_t *cmd, model_t *model, enum ctrl_arl_types type)
{
  ctrl_init( self, cmd, model );
  self->_update = ctrl_arl_update;
  self->_destroy = ctrl_arl_destroy;
  self->_header = ctrl_arl_header;
  self->_writer = ctrl_arl_writer;

  if ( ( self->prp = nalloc( ctrl_arl_prp, 1 ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  ((ctrl_arl_prp *)(self->prp))->type = type;
  ((ctrl_arl_prp *)(self->prp))->sqr_R0 = 0;
  ((ctrl_arl_prp *)(self->prp))->sqr_R = 0;
  ((ctrl_arl_prp *)(self->prp))->delta = 0;
  return self;
}

void ctrl_arl_destroy(ctrl_t *self)
{
  sfree( self->prp );
  ctrl_destroy_default( self );
}

ctrl_t *ctrl_arl_set_params(ctrl_t *self, double k, double beta)
{
  ctrl_arl_set_k( self, k );
  ctrl_arl_set_beta( self, beta );
  return self;
}

ctrl_t *ctrl_arl_update(ctrl_t *self, double t, vec_t p)
{
  self->fz = ctrl_arl_calc_fz( self, t, p );
  return self;
}

void ctrl_arl_header(FILE *fp, void *util)
{
  fprintf( fp, ",type,k,beta,sqr_R0,sqr_R,delta\n" );
}

void ctrl_arl_writer(FILE *fp, ctrl_t *self, void *util)
{
  ctrl_arl_prp *prp;

  prp = self->prp;
  fprintf( fp, ",%d,%f,%f,%f,%f,%f\n",
           ctrl_arl_type(self),
           ctrl_arl_k(self),
           ctrl_arl_beta(self),
           prp->sqr_R0,
           prp->sqr_R,
           prp->delta );
}

double ctrl_arl_calc_sqr_R0(double m, double k, double z0, double zd)
{
  double v0, sqr_R0;
  vec_t p;

  v0 = ctrl_calc_v0( z0, zd );
  p = vec_create_list( 2, zd, v0 );
  sqr_R0 = ctrl_arl_calc_sqr_R( p, m, k, z0 );
  vec_destroy( p );
  return sqr_R0;
}

double ctrl_arl_calc_sqr_R(vec_t p, double m, double k, double z0)
{
  double l, v, c;

  l = vec_elem(p, 0) - z0;
  v = vec_elem(p, 1);
  c = m / k;
  /* Note: definition of c is different from the paper. */
  /* Original definition is c = k / m. */
  return ( l + c * G ) * ( l + c * G ) + c * v * v;
}

double ctrl_arl_calc_delta(vec_t p, double m, double k, double beta, double z0, double zd)
{
  double l, v, sqr_R, sqr_R0;

  l = vec_elem(p, 0) - z0;
  v = vec_elem(p, 1);
  sqr_R = ctrl_arl_calc_sqr_R( p, m, k, z0 );
  sqr_R0 = ctrl_arl_calc_sqr_R0( m, k, z0, zd );
  return beta * l * v * ( sqr_R - sqr_R0 );
}

double ctrl_arl_calc_fz(ctrl_t *self, double t, vec_t p)
{
  double l, delta, fz;

  if( ctrl_phase_in( self, flight ) ) {
    fz = 0;
  } else {
    l = vec_elem(p, 0) - ctrl_z0(self);
    delta = ctrl_arl_delta( self, p );
    fz = -ctrl_arl_k(self) * l + ctrl_model(self)->m * delta;
  }
  return fz;
}