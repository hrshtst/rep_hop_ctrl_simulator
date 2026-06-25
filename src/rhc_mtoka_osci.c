#include "rhc_mtoka_osci.h"
#include "rhc_misc.h"
#include "rhc_ode.h"
#include "rhc_vec.h"

mtoka_osci_neuron_t *mtoka_osci_neuron_init(mtoka_osci_neuron_t *self, int n)
{
  self->tau = 1.0;
  self->T = 1.0;
  self->a = vec_create( n );
  vec_clear( self->a );
  self->b = 0.0;
  self->th = 0.0;
  return self;
}

void mtoka_osci_neuron_destroy(mtoka_osci_neuron_t *self)
{
  vec_destroy( mtoka_osci_neuron_mutual_inhibit_weights(self) );
  mtoka_osci_neuron_mutual_inhibit_weights(self) = NULL;
}

double mtoka_osci_neuron_dxdt(mtoka_osci_neuron_t *self, double x, double v, vec_t y, double c, double s)
{
  return ( -x + c - vec_dot( self->a, y ) - self->b * v + s ) / self->tau;
}

double mtoka_osci_neuron_dvdt(mtoka_osci_neuron_t *self, double v, double y)
{
  return ( -v + y ) / self->T;
}

mtoka_osci_t *mtoka_osci_init(mtoka_osci_t *self, int n_neuron)
{
  int i;

  mtoka_osci_n_neuron(self) = n_neuron;
  if( !( self->neurons = nalloc( mtoka_osci_neuron_t, n_neuron ) ) ||
      !( mtoka_osci_membrane_potential(self) = vec_create( n_neuron ) ) ||
      !( mtoka_osci_firing_rate(self) = vec_create( n_neuron ) ) ||
      !( mtoka_osci_adapt_property(self) = vec_create( n_neuron ) ) ||
      !( mtoka_osci_tonic_input(self) = vec_create( n_neuron ) ) ||
      !( mtoka_osci_sensory_feedback(self) = vec_create( n_neuron ) ) ||
      !( self->xv = vec_create( 2 * n_neuron ) ) ){
    ALLOC_ERR();
    return NULL;
  }
  ode_assign( &self->ode, rk4 );
  if( !ode_init( &self->ode, 2 * n_neuron, mtoka_osci_dp ) ){
    ALLOC_ERR();
    return NULL;
  }
  for( i=0; i<n_neuron; i++ ){
    mtoka_osci_neuron_init( mtoka_osci_neuron( self, i ), n_neuron );
  }
  mtoka_osci_reset( self );
  return self;
}

void mtoka_osci_destroy(mtoka_osci_t *self)
{
  int i;

  for( i=0; i<mtoka_osci_n_neuron(self); i++ ){
    mtoka_osci_neuron_destroy( mtoka_osci_neuron(self,i) );
  }
  vec_destroy( mtoka_osci_membrane_potential(self) );
  mtoka_osci_membrane_potential(self) = NULL;
  vec_destroy( mtoka_osci_firing_rate(self) );
  mtoka_osci_firing_rate(self) = NULL;
  vec_destroy( mtoka_osci_adapt_property(self) );
  mtoka_osci_adapt_property(self) = NULL;
  vec_destroy( mtoka_osci_tonic_input(self) );
  mtoka_osci_tonic_input(self) = NULL;
  vec_destroy( mtoka_osci_sensory_feedback(self) );
  mtoka_osci_sensory_feedback(self) = NULL;
  vec_destroy( self->xv );
  self->xv = NULL;
  if( self->ode._ws )
    ode_destroy( &self->ode );
  sfree( self->neurons );
}

mtoka_osci_t *mtoka_osci_fill_rise_time_const(mtoka_osci_t *self, double tau)
{
  int i;

  for( i=0; i<mtoka_osci_n_neuron(self); i++ )
    mtoka_osci_set_rise_time_const( self, i, tau );
  return self;
}

mtoka_osci_t *mtoka_osci_fill_adapt_time_const(mtoka_osci_t *self, double T)
{
  int i;

  for( i=0; i<mtoka_osci_n_neuron(self); i++ )
    mtoka_osci_set_adapt_time_const( self, i, T );
  return self;
}

static mtoka_osci_t *_mtoka_osci_set_mutual_inhibit_weights_list(mtoka_osci_t *self, int i, va_list args);
mtoka_osci_t *_mtoka_osci_set_mutual_inhibit_weights_list(mtoka_osci_t *self, int i, va_list args)
{
  vec_t a;
  int j;

  a = mtoka_osci_mutual_inhibit_weights(self, i);
  for( j=0; j<mtoka_osci_n_neuron(self); j++ )
    vec_set_elem( a, j, (double)va_arg( args, double ) );
  return self;
}

mtoka_osci_t *mtoka_osci_set_mutual_inhibit_weights_list(mtoka_osci_t *self, int i, ... )
{
  va_list args;

  va_start( args, i );
  _mtoka_osci_set_mutual_inhibit_weights_list( self, i, args );
  va_end( args );
  return self;
}

mtoka_osci_t *mtoka_osci_set_mutual_inhibit_weights_array(mtoka_osci_t *self, int i, double a[])
{
  vec_t v;
  int j;

  v = mtoka_osci_mutual_inhibit_weights( self, i );
  for( j=0; j<mtoka_osci_n_neuron(self); j++ )
    vec_set_elem_array( v, a );
  return self;
}

mtoka_osci_t *mtoka_osci_fill_steady_firing_rate(mtoka_osci_t *self, double b)
{
  int i;

  for( i=0; i<mtoka_osci_n_neuron(self); i++ )
    mtoka_osci_set_steady_firing_rate( self, i, b );
  return self;
}

mtoka_osci_t *mtoka_osci_fill_tonic_input(mtoka_osci_t *self, double c)
{
  vec_fill( mtoka_osci_tonic_input(self), c );
  return self;
}

mtoka_osci_t *mtoka_osci_fill_sensory_feedback(mtoka_osci_t *self, double s)
{
  vec_fill( mtoka_osci_sensory_feedback(self), s );
  return self;
}

mtoka_osci_t *mtoka_osci_fill_firing_threshold(mtoka_osci_t *self, double th)
{
  int i;

  for( i=0; i<mtoka_osci_n_neuron(self); i++ )
    mtoka_osci_set_firing_threshold( self, i, th );
  return self;
}

vec_t mtoka_osci_dp(double t, vec_t xv, void *util, vec_t dxv)
{
  mtoka_osci_t *self = util;
  int n = self->n_neuron;
  mtoka_osci_neuron_t *neuron;
  double x, v, c, s;
  vec_t y;
  double dxdt, dvdt;
  int i;

  y = mtoka_osci_firing_rate(self);
  for( i=0; i<n; i++){
    neuron = mtoka_osci_neuron(self, i);
    x = vec_elem(xv, i);
    v = vec_elem(xv, n+i);
    c = vec_elem(mtoka_osci_tonic_input(self), i);
    s = vec_elem(mtoka_osci_sensory_feedback(self), i);
    dxdt = mtoka_osci_neuron_dxdt( neuron, x, v, y, c, s );
    dvdt = mtoka_osci_neuron_dvdt( neuron, v, vec_elem(y, i) );
    vec_set_elem( dxv, i, dxdt );
    vec_set_elem( dxv, n + i, dvdt );
  }
  return dxv;
}

bool mtoka_osci_reset(mtoka_osci_t *self)
{
  mtoka_osci_time(self) = 0.0;
  mtoka_osci_step(self) = 0;
  vec_clear( self->xv );
  vec_set_elem( self->xv, 0, 0.5 );
  mtoka_osci_update_state( self );
  vec_clear( mtoka_osci_tonic_input(self) );
  vec_clear( mtoka_osci_sensory_feedback(self) );
  return true;
}

void mtoka_osci_update_state(mtoka_osci_t *self)
{
  int n = self->n_neuron;
  int i;

  for( i=0; i<n; i++ ){
    vec_elem(mtoka_osci_membrane_potential(self), i) = vec_elem(self->xv, i);
    vec_elem(mtoka_osci_adapt_property(self), i) = vec_elem(self->xv, n + i);
    vec_elem(mtoka_osci_firing_rate(self), i) = max( vec_elem(self->xv, i), 0.0 );
  }
}

void mtoka_osci_update_time(mtoka_osci_t *self, double dt)
{
  mtoka_osci_inc_step( self );
  mtoka_osci_time(self) = mtoka_osci_step(self) * dt;
}

bool mtoka_osci_update(mtoka_osci_t *self, double dt)
{
  ode_update( &self->ode, mtoka_osci_time(self), self->xv, dt, self );
  mtoka_osci_update_state( self );
  mtoka_osci_update_time( self, dt );
  return true;
}
