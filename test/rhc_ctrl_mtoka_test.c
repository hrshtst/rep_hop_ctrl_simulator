#include "rhc_cmd.h"
#include "rhc_ctrl.h"
#include "rhc_ctrl_mtoka.h"
#include "rhc_model.h"
#include "rhc_mtoka_osci.h"
#include "rhc_test.h"
#include "rhc_vec.h"
#include <stdlib.h>

static cmd_t cmd;
static model_t model;
static ctrl_t ctrl;
static vec_t p;

double frand()
{
  return ((float)rand()/(float)RAND_MAX);
}

void setup()
{
  cmd_default_init( &cmd );
  model_init( &model, 10 );
  ctrl_mtoka_create( &ctrl, &cmd, &model );
  p = vec_create( 2 );
}

void teardown()
{
  vec_destroy( p );
  ctrl_destroy( &ctrl );
  cmd_destroy( &cmd );
}

TEST(test_ctrl_mtoka_create)
{
  int i;
  mtoka_osci_neuron_t *np;

  ASSERT_PTREQ( ctrl_cmd( &ctrl ), &cmd );
  ASSERT_PTREQ( ctrl_model( &ctrl ), &model );
  ASSERT_PTREQ( ctrl_mtoka_update, ctrl._update );
  ASSERT_PTREQ( ctrl_mtoka_destroy, ctrl._destroy );
  ASSERT_PTREQ( ctrl_mtoka_header, ctrl._header );
  ASSERT_PTREQ( ctrl_mtoka_writer, ctrl._writer );
  ASSERT_EQ( 0, ctrl_mtoka_rise_time_const(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_adapt_time_const(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_mutual_inhibit_weights(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_steady_firing_rate(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_steady_firing_rate(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_firing_threshold(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_feedback_gain(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_sensory_gain(&ctrl) );
  ASSERT_EQ( 0, ctrl_mtoka_saturation_gain(&ctrl) );
  ASSERT_PTRNE( NULL, ctrl.prp );
  ASSERT_EQ( 2, mtoka_osci_n_neuron(ctrl_mtoka_osci(&ctrl)) );
  for( i=0; i<2; i++ ){
    np = mtoka_osci_neuron( ctrl_mtoka_osci(&ctrl), i );
    ASSERT_EQ( 1.0, mtoka_osci_neuron_rise_time_const(np) );
    ASSERT_EQ( 1.0, mtoka_osci_neuron_adapt_time_const(np) );
    ASSERT_EQ( 2, vec_size(mtoka_osci_neuron_mutual_inhibit_weights(np)) );
    ASSERT_EQ( 0.0, vec_elem(mtoka_osci_neuron_mutual_inhibit_weights(np), 0) );
    ASSERT_EQ( 0.0, vec_elem(mtoka_osci_neuron_mutual_inhibit_weights(np), 1) );
    ASSERT_EQ( 0.0, mtoka_osci_neuron_steady_firing_rate(np) );
    ASSERT_EQ( 0.0, mtoka_osci_neuron_firing_threshold(np) );
  }
  ASSERT_EQ( 0, ctrl_mtoka_get_prp(&ctrl)->t_prev );
}

TEST(test_ctrl_mtoka_destroy)
{
  ctrl_mtoka_destroy( &ctrl );
  ASSERT_PTREQ( NULL, ctrl_cmd( &ctrl ) );
  ASSERT_PTREQ( NULL, ctrl_model( &ctrl ) );
  ASSERT_PTREQ( NULL, ctrl.prp );
  ASSERT_EQ( 0, ctrl_n( &ctrl ) );
  ASSERT_EQ( 0, ctrl_phi( &ctrl ) );
}

TEST(test_ctrl_mtoka_set_rise_time_const)
{
  double tau = frand();
  ctrl_mtoka_set_rise_time_const( &ctrl, tau );
  ASSERT_EQ( tau, ctrl_mtoka_rise_time_const(&ctrl) );
}

TEST(test_ctrl_mtoka_set_adapt_time_const)
{
  double T = frand();
  ctrl_mtoka_set_adapt_time_const( &ctrl, T );
  ASSERT_EQ( T, ctrl_mtoka_adapt_time_const(&ctrl) );
}

TEST(test_ctrl_mtoka_set_mutual_inhibit_weights)
{
  double a = frand();
  ctrl_mtoka_set_mutual_inhibit_weights( &ctrl, a );
  ASSERT_EQ( a, ctrl_mtoka_mutual_inhibit_weights(&ctrl) );
}

TEST(test_ctrl_mtoka_set_steady_firing_rate)
{
  double b = frand();
  ctrl_mtoka_set_steady_firing_rate( &ctrl, b );
  ASSERT_EQ( b, ctrl_mtoka_steady_firing_rate(&ctrl) );
}

TEST(test_ctrl_mtoka_set_firing_threshold)
{
  double th = frand();
  ctrl_mtoka_set_firing_threshold( &ctrl, th );
  ASSERT_EQ( th, ctrl_mtoka_firing_threshold(&ctrl) );
}

TEST(test_ctrl_mtoka_set_feedback_gain)
{
  double mu = frand();
  ctrl_mtoka_set_feedback_gain( &ctrl, mu );
  ASSERT_EQ( mu, ctrl_mtoka_feedback_gain(&ctrl) );
}

TEST(test_ctrl_mtoka_set_sensory_gain)
{
  double rho = frand();
  ctrl_mtoka_set_sensory_gain( &ctrl, rho );
  ASSERT_EQ( rho, ctrl_mtoka_sensory_gain(&ctrl) );
}

TEST(test_ctrl_mtoka_set_saturation_gain)
{
  double lam = frand();
  ctrl_mtoka_set_saturation_gain( &ctrl, lam );
  ASSERT_EQ( lam, ctrl_mtoka_saturation_gain(&ctrl) );
}

TEST(test_ctrl_mtoka_set_params)
{
  struct case_t{
    double tau, T, a, b, th, mu, rho, lam;
  } cases[] = {
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->tau > 0; c++ ){
    ctrl_mtoka_set_params( &ctrl, c->tau, c->T, c->a, c->b, c->th, c->mu, c->rho, c->lam );
    ASSERT_EQ( c->tau, ctrl_mtoka_rise_time_const(&ctrl) );
    ASSERT_EQ( c->T, ctrl_mtoka_adapt_time_const(&ctrl) );
    ASSERT_EQ( c->a, ctrl_mtoka_mutual_inhibit_weights(&ctrl) );
    ASSERT_EQ( c->b, ctrl_mtoka_steady_firing_rate(&ctrl) );
    ASSERT_EQ( c->th, ctrl_mtoka_firing_threshold(&ctrl) );
    ASSERT_EQ( c->mu, ctrl_mtoka_feedback_gain(&ctrl) );
    ASSERT_EQ( c->rho, ctrl_mtoka_sensory_gain(&ctrl) );
    ASSERT_EQ( c->lam, ctrl_mtoka_saturation_gain(&ctrl) );
  }
}

TEST(test_ctrl_mtoka_update_params)
{
  struct case_t{
    double tau, T, a, b, th, mu, rho, lam;
  } cases[] = {
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  };
  struct case_t *c;
  int i;
  mtoka_osci_neuron_t *np;

  for( c=cases; c->tau>0; c++ ){
    ctrl_mtoka_set_params( &ctrl, c->tau, c->T, c->a, c->b, c->th, c->mu, c->rho, c->lam );
    ctrl_mtoka_update_params( &ctrl );
    for( i=0; i<2; i++ ){
      np = mtoka_osci_neuron( ctrl_mtoka_osci(&ctrl), i );
      ASSERT_EQ( c->tau, mtoka_osci_neuron_rise_time_const(np) );
      ASSERT_EQ( c->T, mtoka_osci_neuron_adapt_time_const(np) );
      ASSERT_EQ( c->a, vec_elem(mtoka_osci_neuron_mutual_inhibit_weights(np), i ? 0:1) );
      ASSERT_EQ( 0, vec_elem(mtoka_osci_neuron_mutual_inhibit_weights(np), i) );
      ASSERT_EQ( c->b, mtoka_osci_neuron_steady_firing_rate(np) );
      ASSERT_EQ( c->th, mtoka_osci_neuron_firing_threshold(np) );
    }
  }
}

TEST(test_ctrl_mtoka_calc_sensory_feedback)
{
  struct case_t{
    double rho, lam, phase, expected;
  } cases[] = {
    { 1.0, 1.0, 1.0, tanh(1.0) },
    { 2.0, 1.0, 1.5, 2.0*tanh(1.5) },
    { 2.0, 0.5, 2.0, 2.0*tanh(1.0) },
    { 0.5, 0.5, 3.0, 0.5*tanh(1.5) },
    { 0.0, 0.0, 0.0, 0.0 },
  };
  struct case_t *c;
  double s;

  for( c=cases; c->rho>0; c++ ){
    ctrl_mtoka_set_sensory_gain( &ctrl, c->rho );
    ctrl_mtoka_set_saturation_gain( &ctrl, c->lam );
    s = ctrl_mtoka_calc_sensory_feedback( &ctrl, c->phase );
    ASSERT_EQ( c->expected, s );
  }
}

TEST(test_ctrl_mtoka_calc_fz)
{
  double mg = model.m * G;
  struct case_t{
    double y1, y2, mu, expected;
  } cases[] = {
    { 1.0, 0.0, 1.0, 1.0 + mg },
    { 1.0, 0.5, 0.5, 0.25 + mg },
    { 0.8, 0.4, 0.4, 0.16 + mg },
    { 0, 0, 0, 0 },
  };
  struct case_t *c;
  double fz;

  for( c=cases; c->expected>0; c++ ){
    vec_set_elem_list( mtoka_osci_firing_rate( ctrl_mtoka_osci(&ctrl) ), c->y1, c->y2 );
    ctrl_mtoka_set_feedback_gain( &ctrl, c->mu );
    fz = ctrl_mtoka_calc_fz( &ctrl, 0, p );
    ASSERT_DOUBLE_EQ( c->expected, fz );
  }
}

TEST(test_ctrl_mtoka_update_check_params)
{
  struct case_t{
    double tau, T, a, b, th, mu, rho, lam;
  } cases[] = {
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand() },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  };
  struct case_t *c;
  int i;
  mtoka_osci_neuron_t *np;

  for( c=cases; c->tau > 0; c++ ){
    ctrl_mtoka_set_params( &ctrl, c->tau, c->T, c->a, c->b, c->th, c->mu, c->rho, c->lam );
    ctrl_mtoka_update( &ctrl, 0, p );
    for( i=0; i<2; i++ ){
      np = mtoka_osci_neuron( ctrl_mtoka_osci(&ctrl), i );
      ASSERT_EQ( c->tau, mtoka_osci_neuron_rise_time_const(np) );
      ASSERT_EQ( c->T, mtoka_osci_neuron_adapt_time_const(np) );
      ASSERT_EQ( c->a, vec_elem(mtoka_osci_neuron_mutual_inhibit_weights(np), i ? 0:1) );
      ASSERT_EQ( 0, vec_elem(mtoka_osci_neuron_mutual_inhibit_weights(np), i) );
      ASSERT_EQ( c->b, mtoka_osci_neuron_steady_firing_rate(np) );
      ASSERT_EQ( c->th, mtoka_osci_neuron_firing_threshold(np) );
    }
  }
}

TEST(test_ctrl_mtoka_update_check_sensory_feedback)
{
  struct case_t{
    double rho, lam;
    double za, zh, zb, z, v;
    double expected;
  } cases[] = {
    { 1.0, 1.0, 0.28, 0.26, 0.24, 0.26, -sqrt(0.04*G), tanh(PI_2)},          /* touchdown */
    { 1.5, 1.0, 0.28, 0.26, 0.24, 0.24, -0.0, 1.5*tanh(PI) },                /* bottom */
    { 1.5, 0.5, 0.28, 0.26, 0.24, 0.26, sqrt(0.04*G), 1.5*tanh(-0.5*PI_2) }, /* lift-off */
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
  };
  struct case_t *c;
  double s1, s2;

  for( c=cases; c->rho>0; c++ ){
    ctrl_mtoka_set_sensory_gain( &ctrl, c->rho );
    ctrl_mtoka_set_saturation_gain( &ctrl, c->lam );
    vec_set_elem_list( p, c->z, c->v );
    ctrl_mtoka_update( &ctrl, 0, p );
    s1 = vec_elem( mtoka_osci_sensory_feedback(ctrl_mtoka_osci(&ctrl)), 0 );
    s2 = vec_elem( mtoka_osci_sensory_feedback(ctrl_mtoka_osci(&ctrl)), 1 );
    ASSERT_EQ( c->expected, s1 );
    ASSERT_EQ( -c->expected, s2 );
  }
}

TEST(test_ctrl_mtoka_update_check_dt)
{
  double dt = 0.01;
  double t = 0;

  ctrl_mtoka_update( &ctrl, t, p );
  ASSERT_EQ( 0, ctrl_mtoka_get_prp(&ctrl)->t_prev );
  ASSERT_EQ( t, mtoka_osci_time(ctrl_mtoka_osci(&ctrl)) );

  t += dt;
  ctrl_mtoka_update( &ctrl, t, p );
  ASSERT_DOUBLE_EQ( t, ctrl_mtoka_get_prp(&ctrl)->t_prev );
  ASSERT_DOUBLE_EQ( t, mtoka_osci_time(ctrl_mtoka_osci(&ctrl)) );

  t += dt;
  ctrl_mtoka_update( &ctrl, t, p );
  ASSERT_DOUBLE_EQ( t, ctrl_mtoka_get_prp(&ctrl)->t_prev );
  ASSERT_DOUBLE_EQ( t, mtoka_osci_time(ctrl_mtoka_osci(&ctrl)) );

  t += dt;
  ctrl_mtoka_update( &ctrl, t, p );
  ASSERT_DOUBLE_EQ( t, ctrl_mtoka_get_prp(&ctrl)->t_prev );
  ASSERT_DOUBLE_EQ( t, mtoka_osci_time(ctrl_mtoka_osci(&ctrl)) );
}

TEST(test_ctrl_mtoka_update_check_osci_update)
{
  double dt = 0.01;
  mtoka_osci_t *osci;
  double mg = model_mass(&model) * G;

  ctrl_mtoka_set_params( &ctrl, 0.25, 0.5, 2.5, 2.5, 0.0, 0.1, 0.1, 0.1 );
  osci = ctrl_mtoka_osci(&ctrl);
  ASSERT_EQ( 0.5, vec_elem(mtoka_osci_membrane_potential(osci), 0) );
  ASSERT_EQ( 0, vec_elem(mtoka_osci_membrane_potential(osci), 1) );
  ASSERT_EQ( 0, vec_elem(mtoka_osci_adapt_property(osci), 0) );
  ASSERT_EQ( 0, vec_elem(mtoka_osci_adapt_property(osci), 1) );
  ASSERT_EQ( 0.5, vec_elem(mtoka_osci_firing_rate(osci), 0) );
  ASSERT_EQ( 0, vec_elem(mtoka_osci_firing_rate(osci), 1) );

  /* Update several times. */
  vec_set_elem_list( p, 0.26, -sqrt(0.04*G) );  /* touchdown */
  ctrl_mtoka_update( &ctrl, 0, p );
  vec_set_elem_list( p, 0.255, -sqrt(0.03*G) );
  ctrl_mtoka_update( &ctrl, dt, p );
  vec_set_elem_list( p, 0.250, -sqrt(0.02*G) );
  ctrl_mtoka_update( &ctrl, 2.0*dt, p );
  vec_set_elem_list( p, 0.245, -sqrt(0.01*G) );
  ctrl_mtoka_update( &ctrl, 3.0*dt, p );

  ASSERT_EQ( 3, mtoka_osci_step(osci) );
  ASSERT_DOUBLE_EQ( 0.03, mtoka_osci_time(osci) );
  /* Check only neuron #1 since #2 is inactive with these params. */
  ASSERT_NE( 0.5, vec_elem(mtoka_osci_membrane_potential(osci), 0) );
  ASSERT_NE( 0, vec_elem(mtoka_osci_adapt_property(osci), 0) );
  ASSERT_NE( 0, vec_elem(mtoka_osci_firing_rate(osci), 0) );
  ASSERT_NE( mg, ctrl_fz(&ctrl) );
}

TEST_SUITE(test_ctrl_mtoka)
{
  CONFIGURE_SUITE(setup, teardown);
  RUN_TEST(test_ctrl_mtoka_create);
  RUN_TEST(test_ctrl_mtoka_destroy);
  RUN_TEST(test_ctrl_mtoka_set_rise_time_const);
  RUN_TEST(test_ctrl_mtoka_set_adapt_time_const);
  RUN_TEST(test_ctrl_mtoka_set_mutual_inhibit_weights);
  RUN_TEST(test_ctrl_mtoka_set_steady_firing_rate);
  RUN_TEST(test_ctrl_mtoka_set_firing_threshold);
  RUN_TEST(test_ctrl_mtoka_set_feedback_gain);
  RUN_TEST(test_ctrl_mtoka_set_sensory_gain);
  RUN_TEST(test_ctrl_mtoka_set_saturation_gain);
  RUN_TEST(test_ctrl_mtoka_set_params);
  RUN_TEST(test_ctrl_mtoka_update_params);
  RUN_TEST(test_ctrl_mtoka_calc_sensory_feedback);
  RUN_TEST(test_ctrl_mtoka_calc_fz);
  RUN_TEST(test_ctrl_mtoka_update_check_params);
  RUN_TEST(test_ctrl_mtoka_update_check_sensory_feedback);
  RUN_TEST(test_ctrl_mtoka_update_check_dt);
  RUN_TEST(test_ctrl_mtoka_update_check_osci_update);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_ctrl_mtoka);
  TEST_REPORT();
  TEST_EXIT();
}
