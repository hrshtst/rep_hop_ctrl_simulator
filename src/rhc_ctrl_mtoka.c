#include "rhc_ctrl.h"
#include "rhc_ctrl_mtoka.h"
#include "rhc_model.h"
#include "rhc_mtoka_osci.h"

ctrl_t *ctrl_mtoka_create(ctrl_t *self, cmd_t *cmd, model_t *model)
{
  ctrl_init( self, cmd, model );
  self->_update = ctrl_mtoka_update;
  self->_destroy = ctrl_mtoka_destroy;
  self->_header = ctrl_mtoka_header;
  self->_writer = ctrl_mtoka_writer;

  if( ( self->prp = nalloc( ctrl_mtoka_prp, 1 ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  mtoka_osci_init( ctrl_mtoka_osci(self), 2 );
  return self;
}

void ctrl_mtoka_destroy(ctrl_t *self)
{
  if( self->prp )
    mtoka_osci_destroy( ctrl_mtoka_osci(self) );
  sfree( self->prp );
  ctrl_destroy_default( self );
}

ctrl_t *ctrl_mtoka_set_sensory_feedback(ctrl_t *self, double s1)
{
  vec_t s;

  s = mtoka_osci_sensory_feedback(ctrl_mtoka_osci(self));
  vec_set_elem( s, 0 ,s1 );
  vec_set_elem( s, 1, -s1);
  return self;
}


ctrl_t *ctrl_mtoka_update(ctrl_t *self, double t, vec_t p)
{
  double s;

  ctrl_update_default( self, t, p );
  ctrl_mtoka_update_params( self );
  s = ctrl_mtoka_calc_sensory_feedback( self, ctrl_phi(self) );
  ctrl_mtoka_set_sensory_feedback( self, s );
  self->fz = ctrl_mtoka_calc_fz( self, t, p );
  return self;
}

void ctrl_mtoka_header(FILE *fp, void *util)
{}

void ctrl_mtoka_writer(FILE *fp, ctrl_t *self, void *util)
{}

ctrl_t *ctrl_mtoka_set_params(ctrl_t *self, double tau, double T, double a, double b, double th, double mu, double rho, double lam)
{
  ctrl_mtoka_set_rise_time_const(self, tau);
  ctrl_mtoka_set_adapt_time_const(self, T);
  ctrl_mtoka_set_mutual_inhibit_weights(self, a);
  ctrl_mtoka_set_steady_firing_rate(self, b);
  ctrl_mtoka_set_firing_threshold(self, th);
  ctrl_mtoka_set_feedback_gain(self, mu);
  ctrl_mtoka_set_sensory_gain(self, rho);
  ctrl_mtoka_set_saturation_gain(self, lam);
  return self;
}

ctrl_t *ctrl_mtoka_update_params(ctrl_t *self)
{
  mtoka_osci_fill_rise_time_const( ctrl_mtoka_osci(self), ctrl_mtoka_rise_time_const(self) );
  mtoka_osci_fill_adapt_time_const( ctrl_mtoka_osci(self), ctrl_mtoka_adapt_time_const(self) );
  mtoka_osci_set_mutual_inhibit_weights_list( ctrl_mtoka_osci(self), 0, 0.0, ctrl_mtoka_mutual_inhibit_weights(self) );
  mtoka_osci_set_mutual_inhibit_weights_list( ctrl_mtoka_osci(self), 1, ctrl_mtoka_mutual_inhibit_weights(self), 0.0 );
  mtoka_osci_fill_steady_firing_rate( ctrl_mtoka_osci(self), ctrl_mtoka_steady_firing_rate(self) );
  mtoka_osci_fill_firing_threshold( ctrl_mtoka_osci(self), ctrl_mtoka_firing_threshold(self) );
  return self;
}

double ctrl_mtoka_calc_sensory_feedback(ctrl_t *self, double phase)
{
  double rho, lambda;

  rho = ctrl_mtoka_sensory_gain(self);
  lambda = ctrl_mtoka_saturation_gain(self);
  return rho * tanh( lambda * phase );
}

double ctrl_mtoka_calc_fz(ctrl_t *self, double t, vec_t p)
{
  vec_t y;
  double mu, mg;

  y = mtoka_osci_firing_rate( ctrl_mtoka_osci(self) );
  mu = ctrl_mtoka_feedback_gain(self);
  mg = model_mass(ctrl_model(self)) * model_gravity(ctrl_model(self));
  return mu * (vec_elem(y, 0) - vec_elem(y, 1)) + mg;
}
