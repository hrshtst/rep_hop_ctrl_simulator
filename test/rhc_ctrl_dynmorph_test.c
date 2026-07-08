#include "rhc_ctrl.h"
#include "rhc_ctrl_dynmorph.h"
#include "rhc_test.h"

static cmd_t cmd;
static model_t model;
static ctrl_t ctrl;
static vec_t p;

void setup()
{
  cmd_default_init( &cmd );
  model_init( &model, 10 );
  ctrl_dynmorph_create( &ctrl, &cmd, &model );
  p = vec_create( 2 );
}

void teardown()
{
  vec_destroy( p );
  ctrl_destroy( &ctrl );
  cmd_destroy( &cmd );
}

TEST(test_ctrl_dynmorph_cmd_init)
{
  ctrl_dynmorph_cmd_init( &ctrl, &cmd );
  ASSERT_EQ( 0.0, ctrl_dynmorph_rho( &ctrl ) );
  ASSERT_EQ( 4.0, ctrl_dynmorph_k( &ctrl ) );
  ASSERT_TRUE( ctrl_dynmorph_soft_landing( &ctrl ) );
}

TEST(test_ctrl_dynmorph_create)
{
  ASSERT_PTREQ( ctrl_cmd( &ctrl ), &cmd );
  ASSERT_PTREQ( ctrl_model( &ctrl ), &model );
  ASSERT_PTREQ( ctrl_dynmorph_update, ctrl._update );
  ASSERT_PTREQ( ctrl_dynmorph_destroy, ctrl._destroy );
  ASSERT_PTREQ( ctrl_dynmorph_header, ctrl._header );
  ASSERT_PTREQ( ctrl_dynmorph_writer, ctrl._writer );
  ASSERT_PTRNE( NULL, ctrl.prp );

  ASSERT_EQ( dynmorph_default, ctrl_dynmorph_type(&ctrl) );
  ASSERT_PTREQ( ctrl_dynmorph_update_params_default, ctrl_dynmorph_get_prp(&ctrl)->_update_params );
  ASSERT_EQ( 0, ctrl_dynmorph_q1(&ctrl) );
  ASSERT_EQ( 0, ctrl_dynmorph_q2(&ctrl) );
  ASSERT_EQ( 0, ctrl_dynmorph_vm(&ctrl) );
  ASSERT_FALSE( ctrl_dynmorph_cushioning(&ctrl) );

  ASSERT_EQ( 0, ctrl_dynmorph_rho(&ctrl) );
  ASSERT_EQ( 4.0, ctrl_dynmorph_k(&ctrl) );
  ASSERT_TRUE( ctrl_dynmorph_soft_landing(&ctrl) );

  ASSERT_EQ( 0.28, ctrl_dynmorph_params_za(&ctrl) );
  ASSERT_EQ( 0.26, ctrl_dynmorph_params_zh(&ctrl) );
  ASSERT_EQ( 0.255, ctrl_dynmorph_params_zm(&ctrl) );
  ASSERT_EQ( 0.23, ctrl_dynmorph_params_zb(&ctrl) );
  ASSERT_EQ( 0, ctrl_dynmorph_params_rho(&ctrl) );
}

TEST(test_ctrl_dynmorph_create_fix_zb)
{
  ctrl_t ctrl_fix_zb;
  ctrl_dynmorph_create_with_type( &ctrl_fix_zb, &cmd, &model, fix_zb );

  ASSERT_PTREQ( ctrl_cmd( &ctrl_fix_zb ), &cmd );
  ASSERT_PTREQ( ctrl_model( &ctrl_fix_zb ), &model );
  ASSERT_PTREQ( ctrl_dynmorph_update, ctrl._update );
  ASSERT_PTREQ( ctrl_dynmorph_destroy, ctrl._destroy );
  ASSERT_PTREQ( ctrl_dynmorph_header, ctrl._header );
  ASSERT_PTREQ( ctrl_dynmorph_writer, ctrl._writer );
  ASSERT_PTRNE( NULL, ctrl.prp );

  ASSERT_EQ( fix_zb, ctrl_dynmorph_type(&ctrl_fix_zb) );
  ASSERT_PTREQ( ctrl_dynmorph_update_params_fix_zb, ctrl_dynmorph_get_prp(&ctrl_fix_zb)->_update_params );
  ASSERT_EQ( 0, ctrl_dynmorph_q1(&ctrl_fix_zb) );
  ASSERT_EQ( 0, ctrl_dynmorph_q2(&ctrl_fix_zb) );
  ASSERT_EQ( 0, ctrl_dynmorph_vm(&ctrl_fix_zb) );

  ASSERT_EQ( 0, ctrl_dynmorph_rho(&ctrl_fix_zb) );
  ASSERT_EQ( 4.0, ctrl_dynmorph_k(&ctrl_fix_zb) );
  ASSERT_TRUE( ctrl_dynmorph_soft_landing(&ctrl_fix_zb) );

  ASSERT_EQ( 0.28, ctrl_dynmorph_params_za(&ctrl_fix_zb) );
  ASSERT_EQ( 0.26, ctrl_dynmorph_params_zh(&ctrl_fix_zb) );
  ASSERT_EQ( 0.255, ctrl_dynmorph_params_zm(&ctrl_fix_zb) );
  ASSERT_EQ( 0.23, ctrl_dynmorph_params_zb(&ctrl_fix_zb) );
  ASSERT_EQ( 0, ctrl_dynmorph_params_rho(&ctrl_fix_zb) );

  ctrl_destroy( &ctrl_fix_zb );
}

TEST(test_ctrl_dynmorph_destroy)
{
  ctrl_dynmorph_destroy( &ctrl );
  ASSERT_PTREQ( NULL, ctrl_cmd( &ctrl ) );
  ASSERT_PTREQ( NULL, ctrl_model( &ctrl ) );
  ASSERT_PTREQ( NULL, ctrl.prp );
  ASSERT_EQ( 0, ctrl_n( &ctrl ) );
  ASSERT_EQ( 0, ctrl_phi( &ctrl ) );
}

TEST(test_ctrl_dynmorph_set_rho)
{
  struct case_t {
    double rho;
  } cases[] = {
    {1.0}, {0.5}, {0.0}, {-1.0}
  };
  struct case_t *c;

  for( c=cases; c->rho>0; c++ ){
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    ASSERT_EQ( c->rho, ctrl_dynmorph_rho(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_set_k)
{
  struct case_t {
    double k;
  } cases[] = {
    {1.0}, {0.5}, {0.0}, {-1.0}
  };
  struct case_t *c;

  for( c=cases; c->k>0; c++ ){
    ctrl_dynmorph_set_k( &ctrl, c->k );
    ASSERT_EQ( c->k, ctrl_dynmorph_k(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_set_soft_landing)
{
  ctrl_dynmorph_enable_soft_landing( &ctrl );
  ASSERT_TRUE( ctrl_dynmorph_soft_landing(&ctrl) );
  ctrl_dynmorph_disable_soft_landing( &ctrl );
  ASSERT_FALSE( ctrl_dynmorph_soft_landing(&ctrl) );
  ctrl_dynmorph_enable_soft_landing( &ctrl );
  ASSERT_TRUE( ctrl_dynmorph_soft_landing(&ctrl) );
  ctrl_dynmorph_disable_soft_landing( &ctrl );
  ASSERT_FALSE( ctrl_dynmorph_soft_landing(&ctrl) );
  ctrl_dynmorph_disable_soft_landing( &ctrl );
  ASSERT_FALSE( ctrl_dynmorph_soft_landing(&ctrl) );
  ctrl_dynmorph_enable_soft_landing( &ctrl );
  ASSERT_TRUE( ctrl_dynmorph_soft_landing(&ctrl) );
}

TEST(test_ctrl_dynmorph_calc_q1)
{
  struct case_t {
    double zh, zm;
    double expected;
  } cases[] = {
    { 1.0+G, 1.0, 1.0, },
    { 2.0, 1.0, sqrt(G), },
    { 1.5, 1.0, sqrt(2.0*G), },
    { 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double q1;

  for( c=cases; c->expected>0.0; c++ ){
    q1=ctrl_dynmorph_calc_q1( c->zh, c->zm, G );
    ASSERT_NEAR( c->expected, q1, 1e-10);
  }
}

TEST(test_ctrl_dynmorph_calc_r)
{
  struct case_t {
    double zm, zb;
    double expected;
  } cases[] = {
    { 1.0, 0.5, 0.5, },
    { 2.0, 1.2, 0.8, },
    { 2.0, 1.0, 1.0, },
    { 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double r;

  for( c=cases; c->expected>0; c++ ){
    r = ctrl_dynmorph_calc_r( c->zm, c->zb );
    ASSERT_NEAR( c->expected, r, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_sqr_vm)
{
  struct case_t {
    double zh, zm, zb;
    double expected;
  } cases[] = {
    { 3.0, 2.0, 1.0, G, },
    { 2.0, 1.5, 0.5, 2.0*G, },
    { 2.0, 1.0, 0.5, 0.25*G, },
    { 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double sqr_vm, vm;

  for( c=cases; c->expected>0; c++ ){
    sqr_vm = ctrl_dynmorph_calc_sqr_vm( c->zh, c->zm, c->zb, G );
    vm = ctrl_dynmorph_calc_vm( c->zh, c->zm, c->zb, G );
    ASSERT_NEAR( c->expected, sqr_vm, 1e-10 );
    ASSERT_NEAR( sqrt( c->expected ), vm, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_sqr_gamma)
{
  struct case_t {
    double z, v, zh, zm, zb;
    double expected;
  } cases[] = {
    { 1.0, 0.0, 1.5+G, 1.5, 0.5, 0.25, },
    { 1.0, 0.5, 2.0, 1.0, 0.5, 1.0/G, },
    { 1.0, -G, 3.0, 2.0, 1.0, 1.0 + G, },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double sqr_gamma, gamma;

  for( c=cases; c->expected>0; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    sqr_gamma = ctrl_dynmorph_calc_sqr_gamma( p, c->zh, c->zm, c->zb, G );
    gamma = ctrl_dynmorph_calc_gamma( p, c->zh, c->zm, c->zb, G );
    ASSERT_NEAR( c->expected, sqr_gamma, 1e-10 );
    ASSERT_NEAR( sqrt( c->expected ), gamma, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_gamma_lc)
{
  struct case_t {
    double rho, k;
    double expected;
  } cases[] = {
    { 1.0, 4.0, 1.0, },              /* full oscillator: the designed orbit */
    { exp(-2.0), 4.0, 0.5, },        /* shrunken cycle */
    { exp(-1.0), 2.0, 0.5, },
    { 0.5, 4.0, 1.0+log(0.5)/4.0, },
    { 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double gamma_lc;

  for( c=cases; c->expected>0; c++ ){
    gamma_lc = ctrl_dynmorph_calc_gamma_lc( c->rho, c->k );
    ASSERT_NEAR( c->expected, gamma_lc, 1e-10 );
  }
  /* at and below rho = exp(-k) the equilibrium is stable: no cycle */
  ASSERT_NEAR( 0.0, ctrl_dynmorph_calc_gamma_lc( exp(-4.0), 4.0 ), 1e-10 );
  ASSERT_TRUE( ctrl_dynmorph_calc_gamma_lc( 0.01, 4.0 ) < 0.0 );
  ASSERT_TRUE( ctrl_dynmorph_calc_gamma_lc( 0.0, 4.0 ) < 0.0 );
}

TEST(test_ctrl_dynmorph_calc_stance_ellipse)
{
  double z[181], vz[181];
  double zh = 0.26, zm = 0.255, zb = 0.24;
  double r, vm, zmax, zmin, vmax;
  int n, i;

  /* rho = 1: the designed orbit, gamma_lc = 1 */
  n = ctrl_dynmorph_calc_stance_ellipse( zh, zm, zb, 1.0, 4.0, 1.0, G, 181, z, vz );
  ASSERT_EQ( 181, n );
  ASSERT_NEAR( z[0], z[n-1], 1e-10 );   /* closed loop */
  ASSERT_NEAR( vz[0], vz[n-1], 1e-10 );
  r = zm - zb;
  vm = ctrl_dynmorph_calc_q1( zh, zm, G ) * r;
  zmax = zmin = z[0];
  vmax = vz[0];
  for( i=0; i<n; i++ ){
    zmax = max( zmax, z[i] );
    zmin = min( zmin, z[i] );
    vmax = max( vmax, vz[i] );
    /* every point satisfies the ellipse equation gamma = 1 */
    ASSERT_NEAR( 1.0, sqr( ( z[i] - zm ) / r ) + sqr( vz[i] / vm ), 1e-10 );
  }
  ASSERT_NEAR( zm + r, zmax, 1e-4 );
  ASSERT_NEAR( zm - r, zmin, 1e-4 );
  ASSERT_NEAR( vm, vmax, 1e-4 );

  /* q_scale stretches the velocity semi-axis only */
  n = ctrl_dynmorph_calc_stance_ellipse( zh, zm, zb, 1.0, 4.0, 2.0, G, 181, z, vz );
  vmax = vz[0];
  for( i=0; i<n; i++ )
    vmax = max( vmax, vz[i] );
  ASSERT_NEAR( 2.0 * vm, vmax, 1e-4 );

  /* no cycle below the bifurcation, degenerate geometry rejected */
  ASSERT_EQ( 0, ctrl_dynmorph_calc_stance_ellipse( zh, zm, zb, 0.0, 4.0, 1.0, G, 181, z, vz ) );
  ASSERT_EQ( 0, ctrl_dynmorph_calc_stance_ellipse( zh, zm, zb, 0.01, 4.0, 1.0, G, 181, z, vz ) );
  ASSERT_EQ( 0, ctrl_dynmorph_calc_stance_ellipse( zh, zm, zm, 1.0, 4.0, 1.0, G, 181, z, vz ) );
  ASSERT_EQ( 0, ctrl_dynmorph_calc_stance_ellipse( zm, zm, zb, 1.0, 4.0, 1.0, G, 181, z, vz ) );
}

TEST(test_ctrl_dynmorph_calc_za)
{
  struct case_t {
    double zh, zm, zb;
    double expected;
  } cases[] = {
    { 2.0, 1.5, 1.0, 2.0, },
    { 3.0, 2.0, 1.0, 3.0, },
    { 3.0, 2.0, 0.5, 3.625, },
    { 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double za;

  for( c=cases; c->expected>0; c++ ){
    za = ctrl_dynmorph_calc_za( c->zh, c->zm, c->zb );
    ASSERT_NEAR( c->expected, za, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_zh)
{
  struct case_t {
    double za, zm, zb;
    double expected;
  } cases[] = {
    { 2.0, 1.5, 1.0, 2.0, },
    { 3.0, 1.5, 1.0, 3.0 - sqrt(2.0), },
    { 3.0, 1.5, 0.5, 3.0 - 0.5 * sqrt(5.0), },
    { 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double zh;

  for( c=cases; c->expected>0; c++ ){
    zh = ctrl_dynmorph_calc_zh( c->za, c->zm, c->zb );
    ASSERT_NEAR( c->expected, zh, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_zm)
{
  struct case_t {
    double za, zh, zb;
    double expected;
  } cases[] = {
    { 2.0, 1.5, 1.0, 1.375, },
    { 3.0, 2.0, 1.0, 1.75, },
    { 3.0, 2.0, 0.5, 1.55, },
    { 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double zm;

  for( c=cases; c->expected>0; c++ ){
    zm = ctrl_dynmorph_calc_zm( c->za, c->zh, c->zb );
    ASSERT_NEAR( c->expected, zm, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_zm_when_za_lower_than_zh)
{
  struct case_t {
    double za, zh, zb;
    double expected;
  } cases[] = {
    { 1.4, 1.5, 1.0, 1.2, },
    { 1.5, 2.0, 1.0, 1.25, },
    { 1.5, 2.0, 0.5, 1.0, },
    { 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double zm;

  for( c=cases; c->expected>0; c++ ){
    zm = ctrl_dynmorph_calc_zm( c->za, c->zh, c->zb );
    ASSERT_NEAR( c->expected, zm, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_calc_zb)
{
  struct case_t {
    double za, zh, zm;
    double expected;
  } cases[] = {
    { 2.0, 1.5, 1.0, 1.0 - 0.5 * sqrt(3.0), },
    { 3.0, 2.0, 1.0, 1.0 - sqrt(3.0), },
    { 3.0, 2.0, 0.5, 0.5 - sqrt(1.5*3.5), },
    { 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;
  double zb;

  for( c=cases; c->za>0.0; c++ ){
    zb = ctrl_dynmorph_calc_zb( c->za, c->zh, c->zm );
    ASSERT_NEAR( c->expected, zb, 1e-10 );
  }
}

TEST(test_ctrl_dynmorph_update_params_hop_fix_zb)
{
  struct case_t {
    double z, v, za, zh, zm, zb, rho;
    double expected_zb;
  } cases[] = {
    /*  z,   v , za,  zh,  zm,  zb, rho, expected_zb */
    { 0.0, 0.0, 2.5, 2.0, 1.5, 0.5, 1.0, 1.5-0.5*sqrt(3), },
    { 0.0, 0.0, 1.5, 1.0, 0.8, 0.2, 1.0, 0.8-0.2*sqrt(6), },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;

  for( c=cases; c->zb>0.0; c++ ){
    cmd_set( &cmd, c->za, c->zh, c->zm, c->zb );
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    ctrl_dynmorph_disable_soft_landing( &ctrl );
    vec_set_elem_list( p, c->z, c->v );
    ctrl_dynmorph_update_params_default( &ctrl, p );
    ASSERT_EQ( c->za, ctrl_dynmorph_params_za(&ctrl) );
    ASSERT_EQ( c->zh, ctrl_dynmorph_params_zh(&ctrl) );
    ASSERT_EQ( c->zm, ctrl_dynmorph_params_zm(&ctrl) );
    ASSERT_NEAR( c->expected_zb, ctrl_dynmorph_params_zb(&ctrl), 1e-10 );
    ASSERT_EQ( c->rho, ctrl_dynmorph_params_rho(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_update_params_hop_fix_zm)
{
  struct case_t {
    double z, v, za, zh, zm, zb, rho;
    double expected_zm;
  } cases[] = {
    /*  z,   v,  za,  zh,  zm,  zb, rho, expected_zm */
    { 0.0, 0.0, 2.5, 2.0, 1.5, 1.0, 1.0, 1.75-1.0/12.0, },
    { 0.0, 0.0, 1.5, 1.0, 0.8, 0.5, 1.0, 0.875, },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;

  for( c=cases; c->zb>0.0; c++ ){
    cmd_set( &cmd, c->za, c->zh, c->zm, c->zb );
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    ctrl_dynmorph_disable_soft_landing( &ctrl );
    vec_set_elem_list( p, c->z, c->v );
    ctrl_dynmorph_update_params_default( &ctrl, p );
    ASSERT_EQ( c->za, ctrl_dynmorph_params_za(&ctrl) );
    ASSERT_EQ( c->zh, ctrl_dynmorph_params_zh(&ctrl) );
    ASSERT_NEAR( c->expected_zm, ctrl_dynmorph_params_zm(&ctrl), 1e-10 );
    ASSERT_EQ( c->zb, ctrl_dynmorph_params_zb(&ctrl) );
    ASSERT_EQ( c->rho, ctrl_dynmorph_params_rho(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_update_params_squat_fix_zm)
{
  struct case_t {
    double z, v, za, zh, zm, zb, rho;
    double expected_zm;
  } cases[] = {
    /*  z,   v,  za,  zh,  zm,  zb, rho, expected_zm */
    { 0.0, 0.0, 1.8, 2.0, 1.5, 1.0, 1.0, 1.4, },
    { 0.0, 0.0, 0.7, 1.0, 0.8, 0.5, 1.0, 0.6, },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;

  for( c=cases; c->zb>0.0; c++ ){
    cmd_set( &cmd, c->za, c->zh, c->zm, c->zb );
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    ctrl_dynmorph_disable_soft_landing( &ctrl );
    vec_set_elem_list( p, c->z, c->v );
    ctrl_dynmorph_update_params_default( &ctrl, p );
    ASSERT_EQ( c->za, ctrl_dynmorph_params_za(&ctrl) );
    ASSERT_EQ( c->zh, ctrl_dynmorph_params_zh(&ctrl) );
    ASSERT_NEAR( c->expected_zm, ctrl_dynmorph_params_zm(&ctrl), 1e-10 );
    ASSERT_EQ( c->zb, ctrl_dynmorph_params_zb(&ctrl) );
    ASSERT_EQ( c->rho, ctrl_dynmorph_params_rho(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_update_params_hop_soft_landing_fix_za)
{
  struct case_t {
    double z, v, za, zh, zm, zb, rho, z_apex;
    double expected_za;
  } cases[] = {
    /*   z,   v , za,  zh,  zm,  zb, rho, z_apex, expected_za */
    { 1.5, -0.5, 2.5, 2.0, 1.55, 0.5, 1.0, 3.0, 3.0, },
    { 0.8, -0.5, 1.5, 1.0, 0.8,  0.8-sqrt(7)/5, 1.0, 1.6, 1.6, },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;

  for( c=cases; c->zb>0.0; c++ ){
    cmd_set( &cmd, c->za, c->zh, c->zm, c->zb );
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    ctrl_dynmorph_enable_soft_landing( &ctrl );

    ctrl_events_init( ctrl_events( &ctrl ) );
    vec_set_elem_list( p, c->z_apex, 0.0 ); /* apex */
    ctrl_events_update( ctrl_events(&ctrl), 1.0, p, &cmd, G );
    vec_set_elem_list( p, c->zh, -0.3 ); /* touchdown */
    ctrl_events_update( ctrl_events(&ctrl), 1.0, p, &cmd, G );
    vec_set_elem_list( p, c->z, c->v ); /* compression */

    ctrl_dynmorph_update_params_default( &ctrl, p );
    ASSERT_NEAR( c->expected_za, ctrl_dynmorph_params_za(&ctrl), 1e-10 );
    ASSERT_EQ( c->zh, ctrl_dynmorph_params_zh(&ctrl) );
    ASSERT_EQ( c->zm, ctrl_dynmorph_params_zm(&ctrl) );
    ASSERT_NEAR( c->zb, ctrl_dynmorph_params_zb(&ctrl), 1e-10 );
    ASSERT_EQ( c->rho, ctrl_dynmorph_params_rho(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_update_params_fix_zb)
{
  struct case_t {
    double z, v, za, zh, zm, zb, rho;
    double expected_zm;
  } cases[] = {
    /*  z,   v,  za,  zh,  zm,  zb, rho, expected_zm */
    { 0.0, 0.0, 2.5, 2.0, 1.5, 1.0, 1.0, 1.75-1.0/12.0, },
    { 0.0, 0.0, 1.5, 1.0, 0.8, 0.5, 1.0, 0.875, },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;

  for( c=cases; c->zb>0.0; c++ ){
    cmd_set( &cmd, c->za, c->zh, c->zm, c->zb );
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    vec_set_elem_list( p, c->z, c->v );
    ctrl_dynmorph_update_params_fix_zb( &ctrl, p );
    ASSERT_EQ( c->za, ctrl_dynmorph_params_za(&ctrl) );
    ASSERT_EQ( c->zh, ctrl_dynmorph_params_zh(&ctrl) );
    ASSERT_NEAR( c->expected_zm, ctrl_dynmorph_params_zm(&ctrl), 1e-10 );
    ASSERT_EQ( c->zb, ctrl_dynmorph_params_zb(&ctrl) );
    ASSERT_EQ( c->rho, ctrl_dynmorph_params_rho(&ctrl) );
  }
}

TEST(test_ctrl_dynmorph_update_params_fix_zm)
{
  struct case_t {
    double z, v, za, zh, zm, zb, rho;
    double expected_zb;
  } cases[] = {
    /*  z,   v,  za,  zh,  zm,  zb, rho, expected_zb */
    { 0.0, 0.0, 2.0, 1.0, 0.8, 0.5, 1.0, 0.8-0.2*sqrt(11), },
    { 0.0, 0.0, 1.5, 1.0, 0.8, 0.5, 1.0, 0.8-0.2*sqrt(6), },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, },
  };
  struct case_t *c;

  for( c=cases; c->zb>0.0; c++ ){
    cmd_set( &cmd, c->za, c->zh, c->zm, c->zb );
    ctrl_dynmorph_set_rho( &ctrl, c->rho );
    vec_set_elem_list( p, c->z, c->v );
    ctrl_dynmorph_update_params_fix_zm( &ctrl, p );
    ASSERT_EQ( c->za, ctrl_dynmorph_params_za(&ctrl) );
    ASSERT_EQ( c->zh, ctrl_dynmorph_params_zh(&ctrl) );
    ASSERT_EQ( c->zm, ctrl_dynmorph_params_zm(&ctrl) );
    ASSERT_NEAR( c->expected_zb, ctrl_dynmorph_params_zb(&ctrl), 1e-10 );
    ASSERT_EQ( c->rho, ctrl_dynmorph_params_rho(&ctrl) );
  }
}

TEST_SUITE(test_ctrl_dynmorph)
{
  CONFIGURE_SUITE( setup, teardown );
  RUN_TEST(test_ctrl_dynmorph_cmd_init);
  RUN_TEST(test_ctrl_dynmorph_create);
  RUN_TEST(test_ctrl_dynmorph_create_fix_zb);
  RUN_TEST(test_ctrl_dynmorph_destroy);
  RUN_TEST(test_ctrl_dynmorph_set_rho);
  RUN_TEST(test_ctrl_dynmorph_set_k);
  RUN_TEST(test_ctrl_dynmorph_set_soft_landing);
  RUN_TEST(test_ctrl_dynmorph_calc_q1);
  RUN_TEST(test_ctrl_dynmorph_calc_r);
  RUN_TEST(test_ctrl_dynmorph_calc_sqr_vm);
  RUN_TEST(test_ctrl_dynmorph_calc_sqr_gamma);
  RUN_TEST(test_ctrl_dynmorph_calc_gamma_lc);
  RUN_TEST(test_ctrl_dynmorph_calc_stance_ellipse);
  RUN_TEST(test_ctrl_dynmorph_calc_za);
  RUN_TEST(test_ctrl_dynmorph_calc_zh);
  RUN_TEST(test_ctrl_dynmorph_calc_zm);
  RUN_TEST(test_ctrl_dynmorph_calc_zm_when_za_lower_than_zh);
  RUN_TEST(test_ctrl_dynmorph_calc_zb);
  RUN_TEST(test_ctrl_dynmorph_update_params_hop_fix_zb);
  RUN_TEST(test_ctrl_dynmorph_update_params_hop_fix_zm);
  RUN_TEST(test_ctrl_dynmorph_update_params_squat_fix_zm);
  RUN_TEST(test_ctrl_dynmorph_update_params_hop_soft_landing_fix_za);
  RUN_TEST(test_ctrl_dynmorph_update_params_fix_zb);
  RUN_TEST(test_ctrl_dynmorph_update_params_fix_zm);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_ctrl_dynmorph);
  TEST_REPORT();
  TEST_EXIT();
}
