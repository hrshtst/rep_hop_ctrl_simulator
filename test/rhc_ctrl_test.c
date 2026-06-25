#include "rhc_cmd.h"
#include "rhc_ctrl.h"
#include "rhc_test.h"
#include "rhc_vec.h"

static cmd_t cmd;
static ctrl_events_t events;
static vec_t p;

void setup_events()
{
  cmd_default_init( &cmd );
  ctrl_events_init( &events );
  p = vec_create( 2 );
}

void teardown_events()
{
  vec_destroy( p );
  ctrl_events_destroy( &events );
  cmd_destroy( &cmd );
}

TEST(test_ctrl_events_init)
{
  ctrl_events_init( &events );
  ASSERT_EQ( 0, ctrl_events_apex(&events)->t );
  ASSERT_EQ( 0, ctrl_events_apex(&events)->z );
  ASSERT_EQ( 0, ctrl_events_apex(&events)->v );
  ASSERT_EQ( 0, ctrl_events_touchdown(&events)->t );
  ASSERT_EQ( 0, ctrl_events_touchdown(&events)->z );
  ASSERT_EQ( 0, ctrl_events_touchdown(&events)->v );
  ASSERT_EQ( 0, ctrl_events_bottom(&events)->t );
  ASSERT_EQ( 0, ctrl_events_bottom(&events)->z );
  ASSERT_EQ( 0, ctrl_events_bottom(&events)->v );
  ASSERT_EQ( 0, ctrl_events_liftoff(&events)->t );
  ASSERT_EQ( 0, ctrl_events_liftoff(&events)->z );
  ASSERT_EQ( 0, ctrl_events_liftoff(&events)->v );
  ASSERT_EQ( invalid, events.phase );
  ASSERT_EQ( 0, ctrl_events_phi(&events) );
  ASSERT_EQ( 0, ctrl_events_n(&events) );
  ASSERT_FALSE( events.is_updated );
}

TEST(test_ctrl_events_destroy)
{
  ctrl_events_destroy( &events );
  ASSERT_EQ( 0, ctrl_events_apex(&events)->t );
  ASSERT_EQ( 0, ctrl_events_apex(&events)->z );
  ASSERT_EQ( 0, ctrl_events_apex(&events)->v );
  ASSERT_EQ( 0, ctrl_events_touchdown(&events)->t );
  ASSERT_EQ( 0, ctrl_events_touchdown(&events)->z );
  ASSERT_EQ( 0, ctrl_events_touchdown(&events)->v );
  ASSERT_EQ( 0, ctrl_events_bottom(&events)->t );
  ASSERT_EQ( 0, ctrl_events_bottom(&events)->z );
  ASSERT_EQ( 0, ctrl_events_bottom(&events)->v );
  ASSERT_EQ( 0, ctrl_events_liftoff(&events)->t );
  ASSERT_EQ( 0, ctrl_events_liftoff(&events)->z );
  ASSERT_EQ( 0, ctrl_events_liftoff(&events)->v );
  ASSERT_EQ( invalid, events.phase );
  ASSERT_EQ( 0, ctrl_events_phi(&events) );
  ASSERT_EQ( 0, ctrl_events_n(&events) );
  ASSERT_FALSE( events.is_updated );
}

TEST(test_ctrl_events_set_apex)
{
  struct case_t {
    double t, z, v;
  } cases[] = {
    { 80.7, 79.8, 49.9 },
    { 91.2, 57.7, 89.4 },
    { 37.7, 32.5, 82.5 },
    { -1, 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->t>0; c++ ){
    ctrl_events_set_apex( &events, c->t, c->z, c->v );
    ASSERT_EQ( c->t, ctrl_events_apex(&events)->t );
    ASSERT_EQ( c->z, ctrl_events_apex(&events)->z );
    ASSERT_EQ( c->v, ctrl_events_apex(&events)->v );
  }
}

TEST(test_ctrl_events_set_touchdown)
{
  struct case_t {
    double t, z, v;
  } cases[] = {
    { 89.5, 51.2, 3.4 },
    { 65.1, 50.6, 23.3 },
    { 50.5, 14.4, 78.2 },
    { -1, 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->t>0; c++ ){
    ctrl_events_set_touchdown( &events, c->t, c->z, c->v );
    ASSERT_EQ( c->t, ctrl_events_touchdown(&events)->t );
    ASSERT_EQ( c->z, ctrl_events_touchdown(&events)->z );
    ASSERT_EQ( c->v, ctrl_events_touchdown(&events)->v );
  }
}

TEST(test_ctrl_events_set_bottom)
{
  struct case_t {
    double t, z, v;
  } cases[] = {
    { 47.1, 50.2, 87.0 },
    { 55.2, 60.5, 69.8 },
    { 78.9, 35.9, 44.9 },
    { -1, 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->t>0; c++ ){
    ctrl_events_set_bottom( &events, c->t, c->z, c->v );
    ASSERT_EQ( c->t, ctrl_events_bottom(&events)->t );
    ASSERT_EQ( c->z, ctrl_events_bottom(&events)->z );
    ASSERT_EQ( c->v, ctrl_events_bottom(&events)->v );
  }
}

TEST(test_ctrl_events_set_liftoff)
{
  struct case_t {
    double t, z, v;
  } cases[] = {
    { 27.9, 5.5, 28.9 },
    { 15.0, 42.8, 71.6 },
    { 72.4, 14.5, 76.0 },
    { -1, 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->t>0; c++ ){
    ctrl_events_set_liftoff( &events, c->t, c->z, c->v );
    ASSERT_EQ( c->t, ctrl_events_liftoff(&events)->t );
    ASSERT_EQ( c->z, ctrl_events_liftoff(&events)->z );
    ASSERT_EQ( c->v, ctrl_events_liftoff(&events)->v );
  }
}

TEST(test_ctrl_events_get_phase_string)
{
  struct case_t {
    enum _ctrl_events_phases_t phase;
    const char *expected;
  } cases[] = {
    { invalid, "invalid" },
    { falling, "falling" },
    { compression, "compression" },
    { extension, "extension" },
    { rising, "rising" },
    { invalid, NULL },
  };
  struct case_t *c;
  char buf[RHC_BUFSIZ];

  for( c=cases; c->expected; c++ ){
    ctrl_events_get_phase_string( c->phase, buf );
    ASSERT_STREQ( c->expected, buf );
  }
}

TEST(test_ctrl_events_calc_phase_complex)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    complex_t expected;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.28, 0.0, { 1.0, 0.0 } },           /* top */
    { 0.28, 0.26, 0.24, 0.26, -sqrt(0.04*G), { 0.0, 1.0 } }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.24, 0.0, { -1.0, 0.0 } },          /* bottom */
    { 0.28, 0.26, 0.24, 0.24, -0.0, { -1.0, 0.0 } },         /* bottom */
    { 0.28, 0.26, 0.24, 0.26, sqrt(0.04*G), { 0.0, -1.0 } }, /* lift-off */
    { 0, 0, 0, 0, 0, { 0, 0 } },
  };
  struct case_t *c;
  complex_t cp;

  for( c=cases; c->za>0; c++ ){
    vec_set_elem_list( p, 2, c->z, c->v );
    ctrl_events_calc_phase_complex( c->zh, c->za, c->zb, p, G, &cp );
    ASSERT_DOUBLE_EQ( c->expected.re, cp.re );
    ASSERT_DOUBLE_EQ( c->expected.im, cp.im );
  }
}

TEST(test_ctrl_events_calc_phi)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    double expected;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.28, 0.0, 0.0 },            /* top */
    { 0.28, 0.26, 0.24, 0.26, -sqrt(0.04*G), PI_2 }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.24, -0.0, PI },            /* bottom */
    { 0.28, 0.26, 0.24, 0.24, 0.0, -PI },            /* bottom */
    { 0.28, 0.26, 0.24, 0.26, sqrt(0.04*G), -PI_2 }, /* lift-off */
    { 0, 0, 0, 0, 0, 0 },
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    vec_set_elem_list( p, 2, c->z, c->v );
    ASSERT_DOUBLE_EQ( c->expected, ctrl_events_calc_phi( c->zh, c->za, c->zb, p, G ) );
  }
}

TEST(test_ctrl_events_is_in_rising)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    bool expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, false },  /* initial */
    { 0.28, 0.26, 0.24, 0.270, -0.010, false },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, false },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, false },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, false },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, true  },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.265,  0.025, true  },  /* rising */
    { 0.28, 0.26, 0.24, 0.270,  0.020, true  },  /* rising */
    { 0.28, 0.26, 0.24, 0.275,  0.010, true  },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, false },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, false },  /* falling */
    {    0,    0,    0,     0,      0, false },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    if( c->expected )
      ASSERT_TRUE( ctrl_events_is_in_rising( &events ) );
    else
      ASSERT_FALSE( ctrl_events_is_in_rising( &events ) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_is_in_falling)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    bool expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, true  },  /* initial */
    { 0.28, 0.26, 0.24, 0.279, -0.001, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.270, -0.010, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.265, -0.020, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, false },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, false },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, false },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, false },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, false },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, true  },  /* apex */
    { 0.28, 0.26, 0.24, 0.279, -0.001, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.270, -0.010, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.265, -0.020, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    {    0,    0,    0,     0,      0, false },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    if( c->expected )
      ASSERT_TRUE( ctrl_events_is_in_falling( &events ) );
    else
      ASSERT_FALSE( ctrl_events_is_in_falling( &events ) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_is_in_flight)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    bool expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, true  },  /* initial */
    { 0.28, 0.26, 0.24, 0.279, -0.001, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.270, -0.010, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.265, -0.020, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, false },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, false },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, false },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, true  },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.265,  0.025, true  },  /* rising */
    { 0.28, 0.26, 0.24, 0.270,  0.020, true  },  /* rising */
    { 0.28, 0.26, 0.24, 0.275,  0.010, true  },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, true  },  /* apex */
    { 0.28, 0.26, 0.24, 0.279, -0.001, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.270, -0.010, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.265, -0.020, true  },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    {    0,    0,    0,     0,      0, false },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    if( c->expected )
      ASSERT_TRUE( ctrl_events_is_in_flight( &events ) );
    else
      ASSERT_FALSE( ctrl_events_is_in_flight( &events ) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_is_in_compression)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    bool expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, false },  /* initial */
    { 0.28, 0.26, 0.24, 0.270, -0.010, false },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, true  },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.255, -0.025, true  },  /* compression */
    { 0.28, 0.26, 0.24, 0.250, -0.020, true  },  /* compression */
    { 0.28, 0.26, 0.24, 0.245, -0.010, true  },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, false },  /* bottom */
    { 0.28, 0.26, 0.24, 0.241,  0.001, false },  /* extension */
    { 0.28, 0.26, 0.24, 0.250,  0.020, false },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, false },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, false },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, false },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, false },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, true  },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.255, -0.025, true  },  /* compression */
    { 0.28, 0.26, 0.24, 0.250, -0.020, true  },  /* compression */
    { 0.28, 0.26, 0.24, 0.245, -0.010, true  },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, false },  /* bottom */
    { 0.28, 0.26, 0.24, 0.241,  0.001, false },  /* extension */
    {    0,    0,    0,     0,      0, false },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    if( c->expected )
      ASSERT_TRUE( ctrl_events_is_in_compression( &events ) );
    else
      ASSERT_FALSE( ctrl_events_is_in_compression( &events ) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_is_in_extension)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    bool expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, false },  /* initial */
    { 0.28, 0.26, 0.24, 0.270, -0.010, false },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, false },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, true  },  /* bottom */
    { 0.28, 0.26, 0.24, 0.241,  0.001, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.245,  0.010, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.250,  0.020, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.255,  0.025, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, false },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, false },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, false },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, false },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, false },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, false },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, true  },  /* bottom */
    { 0.28, 0.26, 0.24, 0.241,  0.001, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.245,  0.010, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.250,  0.020, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.255,  0.025, true  },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, false },  /* lift-off */
    {    0,    0,    0,     0,      0, false },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    if( c->expected )
      ASSERT_TRUE( ctrl_events_is_in_extension( &events ) );
    else
      ASSERT_FALSE( ctrl_events_is_in_extension( &events ) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_phase)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    enum _ctrl_events_phases_t expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, falling     },  /* initial */
    { 0.28, 0.26, 0.24, 0.270, -0.010, falling     },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, compression },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, compression },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, extension   },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, extension   },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, rising      },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, rising      },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, falling     },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, falling     },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, compression },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, compression },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, extension   },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, extension   },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, rising      },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, rising      },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, falling     },  /* apex */
    {    0,    0,    0,     0,      0, invalid     },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    ASSERT_EQ( c->expected, ctrl_events_phase(&events) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_get_phase_string)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    char *expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, "falling"     },  /* initial */
    { 0.28, 0.26, 0.24, 0.270, -0.010, "falling"     },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, "compression" },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, "compression" },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, "extension"   },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, "extension"   },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, "rising"      },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, "rising"      },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, "falling"     },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, "falling"     },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, "compression" },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, "compression" },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, "extension"   },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, "extension"   },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, "rising"      },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, "rising"      },  /* rising */
    { 0.28, 0.26, 0.24, 0.280,  0.000, "falling"     },  /* apex */
    {    0,    0,    0,     0,      0, "invalid"     },  /* terminator */
  };
  struct case_t *c;
  char buf[RHC_BUFSIZ];

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    ctrl_events_get_phase_string( ctrl_events_phase(&events), buf );
    ASSERT_STREQ( c->expected, buf );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_n)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    int expected;
  } cases[] = {
    /* za ,  zh ,  zb ,    z ,     v , expected */
    { 0.28, 0.26, 0.24, 0.280,  0.000, 0 },  /* initial */
    { 0.28, 0.26, 0.24, 0.270, -0.010, 0 },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, 0 },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, 0 },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, 0 },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, 0 },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, 0 },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, 0 },  /* rising */

    { 0.28, 0.26, 0.24, 0.280,  0.000, 1 },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, 1 },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, 1 },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, 1 },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, 1 },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, 1 },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, 1 },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, 1 },  /* rising */
    { 0.28, 0.26, 0.24, 0.279,  0.029, 1 },  /* rising */

    { 0.28, 0.26, 0.24, 0.278, -0.002, 2 },  /* falling */
    { 0.28, 0.26, 0.24, 0.270, -0.010, 2 },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, 2 },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, 2 },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, 2 },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, 2 },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, 2 },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, 2 },  /* rising */
    { 0.28, 0.26, 0.24, 0.278,  0.028, 2 },  /* rising */

    { 0.28, 0.26, 0.24, 0.279, -0.001, 3 },  /* falling */
    { 0.28, 0.26, 0.24, 0.270, -0.010, 3 },  /* falling */
    { 0.28, 0.26, 0.24, 0.260, -0.030, 3 },  /* touchdown */
    { 0.28, 0.26, 0.24, 0.250, -0.020, 3 },  /* compression */
    { 0.28, 0.26, 0.24, 0.240,  0.000, 3 },  /* bottom */
    { 0.28, 0.26, 0.24, 0.250,  0.020, 3 },  /* extension */
    { 0.28, 0.26, 0.24, 0.260,  0.030, 3 },  /* lift-off */
    { 0.28, 0.26, 0.24, 0.270,  0.020, 3 },  /* rising */

    { 0.28, 0.26, 0.24, 0.280,  0.000, 4 },  /* apex */
    { 0.28, 0.26, 0.24, 0.270, -0.010, 4 },  /* falling */

    {    0,    0,    0,     0,      0, 0 },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, 0, p, &cmd, G );
    ASSERT_EQ( c->expected, ctrl_events_n(&events) );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_apex_1)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t apex;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v ,apx:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00, { 0.00, 0.300, 0.00 }, },  /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.300, -0.01, { 0.00, 0.300, 0.00 }, },  /* falling */
    { 0.30, 0.25, 0.20, 0.10, 0.250, -0.30, { 0.00, 0.300, 0.00 }, },  /* touchdown */
    { 0.30, 0.25, 0.20, 0.20, 0.200,  0.00, { 0.00, 0.300, 0.00 }, },  /* bottom */

    { 0.30, 0.25, 0.20, 0.30, 0.250,  sqrt(0.1*G), { 0.30, 0.250, sqrt(0.1*G) }, },  /* lift-off */
    { 0.30, 0.25, 0.20, 0.31, 0.251,  0.29, { 0.31, 0.251, 0.29 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.32, 0.252,  0.28, { 0.32, 0.252, 0.28 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.39, 0.259,  0.01, { 0.39, 0.259, 0.01 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.40, 0.300,  0.00, { 0.40, 0.300, 0.00 }, },  /* apex */

    { 0.30, 0.25, 0.20, 0.41, 0.300, -0.01, { 0.40, 0.300, 0.00 }, },  /* falling */
    { 0.30, 0.25, 0.20, 0.50, 0.250, -0.30, { 0.40, 0.300, 0.00 }, },  /* touchdown */
    { 0.30, 0.25, 0.20, 0.60, 0.200,  0.00, { 0.40, 0.300, 0.00 }, },  /* bottom */

    { 0.30, 0.25, 0.20, 0.70, 0.250,  sqrt(0.1*G), { 0.70, 0.250, sqrt(0.1*G) }, },  /* lift-off */
    { 0.30, 0.25, 0.20, 0.71, 0.251,  0.29, { 0.71, 0.251, 0.29 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.72, 0.252,  0.28, { 0.72, 0.252, 0.28 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.79, 0.259,  0.01, { 0.79, 0.259, 0.01 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.80, 0.300,  0.00, { 0.80, 0.300, 0.00 }, },  /* apex */

    { 0.30, 0.25, 0.20, 0.81, 0.300, -0.01, { 0.80, 0.300, 0.00 }, },  /* falling */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00, { 0.00, 0.000, 0.00 }, },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->apex.t, ctrl_events_apex(&events)->t );
    ASSERT_EQ( c->apex.z, ctrl_events_apex(&events)->z );
    ASSERT_EQ( c->apex.v, ctrl_events_apex(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_apex_2)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t apex;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v ,apx:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00, { 0.00, 0.300,  0.00 }, },  /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.300, -0.01, { 0.00, 0.300,  0.00 }, },  /* falling */
    { 0.30, 0.25, 0.20, 0.10, 0.250, -0.90, { 0.00, 0.300,  0.00 }, },  /* touchdown */
    { 0.30, 0.25, 0.20, 0.20, 0.200,  0.00, { 0.00, 0.300,  0.00 }, },  /* bottom */
    { 0.30, 0.25, 0.20, 0.29, 0.249,  0.90, { 0.00, 0.300,  0.00 }, },  /* extension */

    { 0.30, 0.25, 0.20, 0.30, 0.252,  0.80, { 0.30, 0.252,  0.80 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.31, 0.253,  0.78, { 0.31, 0.253,  0.78 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.39, 0.290,  0.02, { 0.39, 0.290,  0.02 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.40, 0.295,  0.01, { 0.40, 0.295,  0.01 }, },  /* rising */

    { 0.30, 0.25, 0.20, 0.41, 0.280, -0.01, { 0.40, 0.295,  0.01 }, },  /* falling */
    { 0.30, 0.25, 0.20, 0.42, 0.270, -0.02, { 0.40, 0.295,  0.01 }, },  /* falling */
    { 0.30, 0.25, 0.20, 0.50, 0.250, -0.30, { 0.40, 0.295,  0.01 }, },  /* touchdown */
    { 0.30, 0.25, 0.20, 0.60, 0.200,  0.00, { 0.40, 0.295,  0.01 }, },  /* bottom */
    { 0.30, 0.25, 0.20, 0.69, 0.245,  0.80, { 0.40, 0.295,  0.01 }, },  /* extension */

    { 0.30, 0.25, 0.20, 0.70, 0.251,  0.85, { 0.70, 0.251,  0.85 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.71, 0.252,  0.83, { 0.71, 0.252,  0.83 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.79, 0.259,  0.05, { 0.79, 0.259,  0.05 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.80, 0.280,  0.03, { 0.80, 0.280,  0.03 }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.81, 0.295, -0.01, { 0.81, 0.295, -0.01 }, },  /* falling */

    { 0.30, 0.25, 0.20, 0.82, 0.290, -0.02, { 0.81, 0.295, -0.01 }, },  /* falling */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00, { 0.00, 0.000,  0.00 }, },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->apex.t, ctrl_events_apex(&events)->t );
    ASSERT_EQ( c->apex.z, ctrl_events_apex(&events)->z );
    ASSERT_EQ( c->apex.v, ctrl_events_apex(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_apex_initial)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t apex;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v ,apx:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00, { 0.00, 0.300, 0.00 }, },  /* initial */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00, { 0.00, 0.000, 0.00 }, },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->apex.t, ctrl_events_apex(&events)->t );
    ASSERT_EQ( c->apex.z, ctrl_events_apex(&events)->z );
    ASSERT_EQ( c->apex.v, ctrl_events_apex(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_touchdown_1)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t touchdown;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , td:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.300,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.01, 0.299, -0.01 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.09, 0.255, -0.90,
                      { 0.09, 0.255, -0.90 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.10, 0.250, -sqrt(0.1*G),
                      { 0.10, 0.250, -sqrt(0.1*G) }, },  /* touchdown */

    { 0.30, 0.25, 0.20, 0.11, 0.249, -0.90,
                      { 0.10, 0.250, -sqrt(0.1*G) }, },  /* compression */
    { 0.30, 0.25, 0.20, 0.20, 0.200,  0.00,
                      { 0.10, 0.250, -sqrt(0.1*G) }, },  /* bottom */
    { 0.30, 0.25, 0.20, 0.30, 0.250,  sqrt(0.1*G),
                      { 0.10, 0.250, -sqrt(0.1*G) }, },  /* lift-off */

    { 0.30, 0.25, 0.20, 0.40, 0.300,  0.00,
                      { 0.40, 0.300,  0.00 }, },         /* apex */
    { 0.30, 0.25, 0.20, 0.41, 0.299, -0.01,
                      { 0.41, 0.299, -0.01 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.49, 0.255, -0.90,
                      { 0.49, 0.255, -0.90 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.50, 0.250, -sqrt(0.1*G),
                      { 0.50, 0.250, -sqrt(0.1*G) }, },  /* touchdown */

    { 0.30, 0.25, 0.20, 0.51, 0.249, -0.90,
                      { 0.50, 0.250, -sqrt(0.1*G) }, },  /* compression */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00, { 0.00, 0.000, 0.00 }, },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->touchdown.t, ctrl_events_touchdown(&events)->t );
    ASSERT_EQ( c->touchdown.z, ctrl_events_touchdown(&events)->z );
    ASSERT_EQ( c->touchdown.v, ctrl_events_touchdown(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_touchdown_2)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t touchdown;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , td:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.300,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.01, 0.299, -0.01 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.09, 0.260, -0.80,
                      { 0.09, 0.260, -0.80 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.10, 0.249, -0.90,
                      { 0.10, 0.249, -0.90 }, },         /* compression */

    { 0.30, 0.25, 0.20, 0.11, 0.240, -0.80,
                      { 0.10, 0.249, -0.90 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.20, 0.200,  0.00,
                      { 0.10, 0.249, -0.90 }, },         /* bottom */
    { 0.30, 0.25, 0.20, 0.30, 0.250,  sqrt(0.1*G),
                      { 0.10, 0.249, -0.90 }, },         /* lift-off */
    { 0.30, 0.25, 0.20, 0.39, 0.299,  0.01,
                      { 0.10, 0.249, -0.90 }, },         /* rising */

    { 0.30, 0.25, 0.20, 0.40, 0.295, -0.02,
                      { 0.40, 0.295, -0.02 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.41, 0.280, -0.04,
                      { 0.41, 0.280, -0.04 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.49, 0.251, -0.90,
                      { 0.49, 0.251, -0.90 }, },         /* falling */

    { 0.30, 0.25, 0.20, 0.50, 0.248, -0.80,
                      { 0.49, 0.251, -0.90 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.60, 0.200,  0.00,
                      { 0.49, 0.251, -0.90 }, },         /* bottom */
    { 0.30, 0.25, 0.20, 0.70, 0.250,  sqrt(0.1*G),
                      { 0.49, 0.251, -0.90 }, },         /* lift-off */
    { 0.30, 0.25, 0.20, 0.79, 0.290,  0.03,
                      { 0.49, 0.251, -0.90 }, },         /* rising */

    { 0.30, 0.25, 0.20, 0.80, 0.295, -0.01,
                      { 0.80, 0.295, -0.01 }, },         /* falling */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->touchdown.t, ctrl_events_touchdown(&events)->t );
    ASSERT_EQ( c->touchdown.z, ctrl_events_touchdown(&events)->z );
    ASSERT_EQ( c->touchdown.v, ctrl_events_touchdown(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_touchdown_initial)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t touchdown;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , td:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.250, -sqrt(0.1*G),
                      { 0.00, 0.250, -sqrt(0.1*G) }, },  /* touchdown */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00, { 0.00, 0.000, 0.00 }, },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->touchdown.t, ctrl_events_touchdown(&events)->t );
    ASSERT_EQ( c->touchdown.z, ctrl_events_touchdown(&events)->z );
    ASSERT_EQ( c->touchdown.v, ctrl_events_touchdown(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_bottom_1)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t bottom;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v ,btm:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.00, 0.000,  0.00 }, },         /* falling */

    { 0.30, 0.25, 0.20, 0.10, 0.250, -sqrt(0.1*G),
                      { 0.10, 0.250, -sqrt(0.1*G) }, },  /* touchdown */
    { 0.30, 0.25, 0.20, 0.11, 0.249, -0.90,
                      { 0.11, 0.249, -0.90 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.19, 0.201, -0.01,
                      { 0.19, 0.201, -0.01 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.20, 0.200,  0.00,
                      { 0.20, 0.200,  0.00 }, },         /* bottom */

    { 0.30, 0.25, 0.20, 0.21, 0.201,  0.01,
                      { 0.20, 0.200,  0.00 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.30, 0.250,  sqrt(0.1*G),
                      { 0.20, 0.200,  0.00 }, },         /* lift-off */
    { 0.30, 0.25, 0.20, 0.40, 0.300,  0.00,
                      { 0.20, 0.200,  0.00 }, },         /* apex */
    { 0.30, 0.25, 0.20, 0.49, 0.255, -0.90,
                      { 0.20, 0.200,  0.00 }, },         /* falling */

    { 0.30, 0.25, 0.20, 0.50, 0.250, -sqrt(0.1*G),
                      { 0.50, 0.250, -sqrt(0.1*G) }, },  /* touchdown */
    { 0.30, 0.25, 0.20, 0.51, 0.249, -0.90,
                      { 0.51, 0.249, -0.90 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.59, 0.201, -0.01,
                      { 0.59, 0.201, -0.01 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.60, 0.200,  0.00,
                      { 0.60, 0.200,  0.00 }, },         /* bottom */

    { 0.30, 0.25, 0.20, 0.61, 0.201,  0.01,
                      { 0.60, 0.200,  0.00 }, },         /* extension */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->bottom.t, ctrl_events_bottom(&events)->t );
    ASSERT_EQ( c->bottom.z, ctrl_events_bottom(&events)->z );
    ASSERT_EQ( c->bottom.v, ctrl_events_bottom(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_bottom_initial)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t bottom;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v ,btm:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.200,  0.00,
                      { 0.00, 0.200,  0.00 }, },         /* bottom */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00, { 0.00, 0.000, 0.00 }, },  /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->bottom.t, ctrl_events_bottom(&events)->t );
    ASSERT_EQ( c->bottom.z, ctrl_events_bottom(&events)->z );
    ASSERT_EQ( c->bottom.v, ctrl_events_bottom(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_bottom_2)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t bottom;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v ,btm:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.00, 0.000,  0.00 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.09, 0.251, -0.90,
                      { 0.00, 0.000,  0.00 }, },         /* falling */

    { 0.30, 0.25, 0.20, 0.10, 0.248, -0.80,
                      { 0.10, 0.248, -0.80 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.11, 0.240, -0.70,
                      { 0.11, 0.240, -0.70 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.19, 0.201, -0.01,
                      { 0.19, 0.201, -0.01 }, },         /* compression */

    { 0.30, 0.25, 0.20, 0.20, 0.205,  0.02,
                      { 0.19, 0.201, -0.01 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.22, 0.210,  0.05,
                      { 0.19, 0.201, -0.01 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.30, 0.250,  sqrt(0.1*G),
                      { 0.19, 0.201, -0.01 }, },         /* lift-off */
    { 0.30, 0.25, 0.20, 0.40, 0.300,  0.00,
                      { 0.19, 0.201, -0.01 }, },         /* apex */
    { 0.30, 0.25, 0.20, 0.49, 0.255, -0.80,
                      { 0.19, 0.201, -0.01 }, },         /* falling */

    { 0.30, 0.25, 0.20, 0.50, 0.249, -0.90,
                      { 0.50, 0.249, -0.90 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.51, 0.240, -0.70,
                      { 0.51, 0.240, -0.70 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.59, 0.205, -0.02,
                      { 0.59, 0.205, -0.02 }, },         /* compression */
    { 0.30, 0.25, 0.20, 0.60, 0.201,  0.01,
                      { 0.60, 0.201,  0.01 }, },         /* extension */

    { 0.30, 0.25, 0.20, 0.61, 0.205,  0.02,
                      { 0.60, 0.201,  0.01 }, },         /* extension */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->bottom.t, ctrl_events_bottom(&events)->t );
    ASSERT_EQ( c->bottom.z, ctrl_events_bottom(&events)->z );
    ASSERT_EQ( c->bottom.v, ctrl_events_bottom(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_liftoff_1)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t liftoff;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , lo:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.00, 0.000,  0.00 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.10, 0.250, -sqrt(0.1*G),
                      { 0.00, 0.000,  0.00 }, },         /* touchdown */

    { 0.30, 0.25, 0.20, 0.20, 0.200,  0.00,
                      { 0.20, 0.200,  0.00 }, },         /* bottom */
    { 0.30, 0.25, 0.20, 0.21, 0.201,  0.01,
                      { 0.21, 0.201,  0.01 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.29, 0.249,  0.90,
                      { 0.29, 0.249,  0.90 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.30, 0.250,  sqrt(0.1*G),
                      { 0.30, 0.250,  sqrt(0.1*G) }, },  /* lift-off */

    { 0.30, 0.25, 0.20, 0.31, 0.251,  0.90,
                      { 0.30, 0.250,  sqrt(0.1*G) }, },  /* rising */
    { 0.30, 0.25, 0.20, 0.40, 0.300,  0.00,
                      { 0.30, 0.250,  sqrt(0.1*G) }, },  /* apex */
    { 0.30, 0.25, 0.20, 0.50, 0.250, -sqrt(0.1*G),
                      { 0.30, 0.250,  sqrt(0.1*G) }, },  /* touchdown */

    { 0.30, 0.25, 0.20, 0.60, 0.200,  0.00,
                      { 0.60, 0.200,  0.00 }, },         /* bottom */
    { 0.30, 0.25, 0.20, 0.61, 0.201,  0.01,
                      { 0.61, 0.201,  0.01 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.69, 0.249,  0.90,
                      { 0.69, 0.249,  0.90 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.70, 0.250,  sqrt(0.1*G),
                      { 0.70, 0.250,  sqrt(0.1*G) }, },  /* lift-off */

    { 0.30, 0.25, 0.20, 0.71, 0.251,  0.90,
                      { 0.70, 0.250,  sqrt(0.1*G) }, },  /* rising */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->liftoff.t, ctrl_events_liftoff(&events)->t );
    ASSERT_EQ( c->liftoff.z, ctrl_events_liftoff(&events)->z );
    ASSERT_EQ( c->liftoff.v, ctrl_events_liftoff(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_liftoff_2)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t liftoff;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , lo:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.00, 0.000,  0.00 }, },         /* falling */
    { 0.30, 0.25, 0.20, 0.10, 0.250, -sqrt(0.1*G),
                      { 0.00, 0.000,  0.00 }, },         /* touchdown */
    { 0.30, 0.25, 0.20, 0.19, 0.201, -0.01,
                      { 0.00, 0.000,  0.00 }, },         /* compression */

    { 0.30, 0.25, 0.20, 0.20, 0.202,  0.02,
                      { 0.20, 0.202,  0.02 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.21, 0.205,  0.04,
                      { 0.21, 0.205,  0.04 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.29, 0.249,  0.90,
                      { 0.29, 0.249,  0.90 }, },         /* extension */

    { 0.30, 0.25, 0.20, 0.30, 0.252,  0.85,
                      { 0.29, 0.249,  0.90 }, },         /* rising */
    { 0.30, 0.25, 0.20, 0.31, 0.255,  0.80,
                      { 0.29, 0.249,  0.90 }, },         /* rising */
    { 0.30, 0.25, 0.20, 0.40, 0.300,  0.00,
                      { 0.29, 0.249,  0.90 }, },         /* apex */
    { 0.30, 0.25, 0.20, 0.50, 0.250, -sqrt(0.1*G),
                      { 0.29, 0.249,  0.90 }, },         /* touchdown */
    { 0.30, 0.25, 0.20, 0.59, 0.195, -0.05,
                      { 0.29, 0.249,  0.90 }, },         /* compression */

    { 0.30, 0.25, 0.20, 0.60, 0.201,  0.01,
                      { 0.60, 0.201,  0.01 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.61, 0.205,  0.02,
                      { 0.61, 0.205,  0.02 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.69, 0.248,  0.80,
                      { 0.69, 0.248,  0.80 }, },         /* extension */
    { 0.30, 0.25, 0.20, 0.70, 0.251,  0.90,
                      { 0.70, 0.251,  0.90, } },         /* rising */

    { 0.30, 0.25, 0.20, 0.71, 0.252,  0.85,
                      { 0.70, 0.251,  0.90 }, },         /* rising */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->liftoff.t, ctrl_events_liftoff(&events)->t );
    ASSERT_EQ( c->liftoff.z, ctrl_events_liftoff(&events)->z );
    ASSERT_EQ( c->liftoff.v, ctrl_events_liftoff(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_liftoff_initial)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t liftoff;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , lo:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.250,  sqrt(0.1*G),
                      { 0.00, 0.250,  sqrt(0.1*G) }, },  /* lift-off */
    { 0.00, 0.00, 0.00, 0.00, 0.000,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* terminator */
  };
  struct case_t *c;

  for( c=cases; c->za>0; c++ ){
    cmd.za = c->za;
    cmd.zh = c->zh;
    cmd.zb = c->zb;
    vec_set_elem_list( p, c->z, c->v );
    ctrl_events_update( &events, c->t, p, &cmd, G );
    ASSERT_EQ( c->liftoff.t, ctrl_events_liftoff(&events)->t );
    ASSERT_EQ( c->liftoff.z, ctrl_events_liftoff(&events)->z );
    ASSERT_EQ( c->liftoff.v, ctrl_events_liftoff(&events)->v );
    ctrl_events_update_next( &events );
  }
}

TEST(test_ctrl_events_update_flag)
{
  struct case_t {
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expected recorded events */
    struct _ctrl_events_tuple_t liftoff;
  } cases[] = {
    /* za ,  zh ,  zb ,   t ,    z ,    v , lo:{ t,    z ,   v } */
    { 0.30, 0.25, 0.20, 0.00, 0.300,  0.00,
                      { 0.00, 0.000,  0.00 }, },         /* initial */
    { 0.30, 0.25, 0.20, 0.01, 0.299, -0.01,
                      { 0.00, 0.000,  0.00 }, },         /* falling */
  };
  struct case_t *c;
  double phi;

  c = cases;
  cmd.za = c->za;
  cmd.zh = c->zh;
  cmd.zb = c->zb;
  vec_set_elem_list( p, c->z, c->v );

  ASSERT_FALSE( events.is_updated );
  ctrl_events_update( &events, c->t, p, &cmd, G );
  phi = ctrl_events_phi( &events );

  c++;
  vec_set_elem_list( p, c->z, c->v );
  ASSERT_TRUE( events.is_updated );
  ctrl_events_update( &events, c->t, p, &cmd, G );
  ASSERT_EQ( phi, ctrl_events_phi( &events ) );
}

TEST_SUITE(test_ctrl_events)
{
  CONFIGURE_SUITE(setup_events, teardown_events);
  RUN_TEST(test_ctrl_events_init);
  RUN_TEST(test_ctrl_events_destroy);
  RUN_TEST(test_ctrl_events_set_apex);
  RUN_TEST(test_ctrl_events_set_touchdown);
  RUN_TEST(test_ctrl_events_set_bottom);
  RUN_TEST(test_ctrl_events_set_liftoff);
  RUN_TEST(test_ctrl_events_get_phase_string);
  RUN_TEST(test_ctrl_events_calc_phase_complex);
  RUN_TEST(test_ctrl_events_calc_phi);
  RUN_TEST(test_ctrl_events_is_in_rising);
  RUN_TEST(test_ctrl_events_is_in_falling);
  RUN_TEST(test_ctrl_events_is_in_flight);
  RUN_TEST(test_ctrl_events_is_in_compression);
  RUN_TEST(test_ctrl_events_is_in_extension);
  RUN_TEST(test_ctrl_events_update_phase);
  RUN_TEST(test_ctrl_events_update_get_phase_string);
  RUN_TEST(test_ctrl_events_update_n);
  RUN_TEST(test_ctrl_events_update_apex_1);
  RUN_TEST(test_ctrl_events_update_apex_2);
  RUN_TEST(test_ctrl_events_update_apex_initial);
  RUN_TEST(test_ctrl_events_update_touchdown_1);
  RUN_TEST(test_ctrl_events_update_touchdown_2);
  RUN_TEST(test_ctrl_events_update_touchdown_initial);
  RUN_TEST(test_ctrl_events_update_bottom_1);
  RUN_TEST(test_ctrl_events_update_bottom_2);
  RUN_TEST(test_ctrl_events_update_bottom_initial);
  RUN_TEST(test_ctrl_events_update_liftoff_1);
  RUN_TEST(test_ctrl_events_update_liftoff_2);
  RUN_TEST(test_ctrl_events_update_liftoff_initial);
  RUN_TEST(test_ctrl_events_update_flag);
}


static model_t model;
static ctrl_t ctrl;

void setup()
{
  cmd_default_init( &cmd );
  model_init( &model, 10 );
  ctrl_init( &ctrl, &cmd, &model );
  p = vec_create( 2 );
}

void teardown()
{
  vec_destroy( p );
  ctrl_destroy( &ctrl );
  cmd_destroy( &cmd );
}

TEST(test_ctrl_init)
{
  ASSERT_PTREQ( &cmd, ctrl_cmd( &ctrl ) );
  ASSERT_PTREQ( &model, ctrl_model( &ctrl ) );
  ASSERT_EQ( 0, ctrl_fz( &ctrl ) );
  ASSERT_PTREQ( ctrl_reset_default, ctrl._reset );
  ASSERT_PTREQ( ctrl_update_default, ctrl._update );
  ASSERT_PTREQ( ctrl_destroy_default, ctrl._destroy );
  ASSERT_PTREQ( ctrl_header_default, ctrl._header );
  ASSERT_PTREQ( ctrl_writer_default, ctrl._writer );
  ASSERT_PTREQ( NULL, ctrl.prp);
  ASSERT_EQ( 0, ctrl_n( &ctrl ) );
  ASSERT_EQ( 0, ctrl_phi( &ctrl ) );
  ASSERT_EQ( invalid, ctrl_events_phase( ctrl_events( &ctrl ) ) );
}

TEST(test_ctrl_destroy)
{
  ctrl_destroy( &ctrl );
  ASSERT_PTREQ( NULL, ctrl_cmd( &ctrl ) );
  ASSERT_PTREQ( NULL, ctrl_model( &ctrl ) );
  ASSERT_PTREQ( NULL, ctrl.prp );
  ASSERT_EQ( 0, ctrl_n( &ctrl ) );
  ASSERT_EQ( 0, ctrl_phi( &ctrl ) );
}

TEST(test_ctrl_za)
{
  double cases[] = { 0.28, 0.3, 0.32, 0.0 };
  double *c;

  for( c=cases; *c>0; c++ ){
    cmd.za = *c;
    ASSERT_EQ( *c, ctrl_za( &ctrl ) );
  }
}

TEST(test_ctrl_zh)
{
  double cases[] = { 0.26, 0.28, 0.3, 0.0 };
  double *c;

  for( c=cases; *c>0; c++ ){
    cmd.zh = *c;
    ASSERT_EQ( *c, ctrl_zh( &ctrl ) );
  }
}

TEST(test_ctrl_zm)
{
  double cases[] = { 0.26, 0.28, 0.3, 0.0 };
  double *c;

  for( c=cases; *c>0; c++ ){
    cmd.zm = *c;
    ASSERT_EQ( *c, ctrl_zm( &ctrl ) );
  }
}

TEST(test_ctrl_zb)
{
  double cases[] = { 0.24, 0.26, 0.28, 0.0 };
  double *c;

  for( c=cases; *c>0; c++ ){
    cmd.zb = *c;
    ASSERT_EQ( *c, ctrl_zb( &ctrl ) );
  }
}

TEST(test_ctrl_vh)
{
  struct case_t {
    double zh, za;
    double expected;
  } cases[] = {
    { 0.26, 0.28, sqrt(0.04*G) },
    { 0.25, 0.28, sqrt(0.06*G) },
    { 0.27, 0.3,  sqrt(0.06*G) },
    { 0, 0, 0 }
  };
  struct case_t *c;

  for( c=cases; c->zh>0; c++ ){
    cmd.zh = c->zh;
    cmd.za = c->za;
    ASSERT_DOUBLE_EQ( c->expected, ctrl_vh( &ctrl ) );
  }
}

TEST(test_ctrl_reset_default)
{
  vec_set_elem_list( p, 2, 0.24, 0.0 );
  ctrl_update( &ctrl, 0.0, p );
  ctrl_fz( &ctrl ) = 2.34;  /* random */
  ASSERT_NE( 0.0, ctrl_fz(&ctrl) );
  ASSERT_NE( invalid, ctrl_phase(&ctrl) );
  ctrl_reset( &ctrl, NULL );
  ASSERT_EQ( 0.0, ctrl_fz(&ctrl) );
  ASSERT_EQ( invalid, ctrl_phase(&ctrl) );
}

TEST(test_ctrl_update_default)
{
  struct case_t {
    double za, zh, zb;
    double z, v;
    int expct_n;
    double expct_phi;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.28, 0.0, 0, 0.0 },            /* top */
    { 0.28, 0.26, 0.24, 0.26, -sqrt(0.04*G), 0, PI_2 }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.24, -0.0, 0, PI },            /* bottom */
    { 0.28, 0.26, 0.24, 0.24, 0.0, 0, -PI },            /* bottom */
    { 0.28, 0.26, 0.24, 0.26, sqrt(0.04*G), 0, -PI_2 }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.28, 0.0, 1, 0.0 },            /* top */
    { 0.28, 0.26, 0.24, 0.26, -sqrt(0.04*G), 1, PI_2 }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.24, -0.0, 1, PI },            /* bottom */
    { 0.28, 0.26, 0.24, 0.24, 0.0, 1, -PI },            /* bottom */
    { 0.28, 0.26, 0.24, 0.26, sqrt(0.04*G), 1, -PI_2 }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.28, 0.0, 2, 0.0 },            /* top */
    { 0, 0, 0, 0, 0, 0, 0 },
  };
  struct case_t *c;
  double t;

  t = 0;
  for( c=cases; c->za>0; c++ ){
    vec_set_elem_list( p, 2, c->z, c->v );
    ctrl_update( &ctrl, t, p );
    ASSERT_DOUBLE_EQ( c->expct_n, ctrl_n( &ctrl ) );
    ASSERT_DOUBLE_EQ( c->expct_phi, ctrl_phi( &ctrl ) );
    t += 0.01;
  }
}

TEST(test_ctrl_phase)
{
  double vh = sqrt( 0.04 * G );
  struct case_t{
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expectations */
    enum _ctrl_events_phases_t phase;
    bool flight, falling, compression, extension, rising;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.00, 0.28,  0.00, falling, true, true, false, false, false }, /* apex */
    { 0.28, 0.26, 0.24, 0.06, 0.27, -0.30, falling, true, true, false, false, false }, /* falling */
    { 0.28, 0.26, 0.24, 0.12, 0.26, -vh,   compression, false, false, true, false, false }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.18, 0.25, -0.30, compression, false, false, true, false, false }, /* compression */
    { 0.28, 0.26, 0.24, 0.24, 0.24,  0.00, extension, false, false, false, true, false }, /* bottom */
    { 0.28, 0.26, 0.24, 0.30, 0.25,  0.30, extension, false, false, false, true, false }, /* extension */
    { 0.28, 0.26, 0.24, 0.36, 0.26,  vh,   rising, true, false, false, false, true }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.42, 0.27,  0.30, rising, true, false, false, false, true }, /* rising */
    { 0.28, 0.26, 0.24, 0.48, 0.28,  0.00, falling, true, true, false, false, false }, /* apex */
    { 0, 0, 0, 0, 0, 0, invalid, false, false, false, false, false },
  };
  struct case_t *c;

  for( c=cases; c->phase != invalid; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    ctrl_update( &ctrl, c->t, p );
    ASSERT_EQ( c->phase, ctrl_phase( &ctrl ) );
    ASSERT_EQ( c->flight, ctrl_phase_in( &ctrl, flight ) );
    ASSERT_EQ( c->falling, ctrl_phase_in( &ctrl, falling ) );
    ASSERT_EQ( c->compression, ctrl_phase_in( &ctrl, compression ) );
    ASSERT_EQ( c->extension, ctrl_phase_in( &ctrl, extension ) );
    ASSERT_EQ( c->rising, ctrl_phase_in( &ctrl, rising ) );
  }
}

TEST(test_ctrl_phase_string)
{
  double vh = sqrt( 0.04 * G );
  struct case_t{
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expectations */
    char *expected;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.00, 0.28,  0.00, "falling" }, /* apex */
    { 0.28, 0.26, 0.24, 0.06, 0.27, -0.30, "falling" }, /* falling */
    { 0.28, 0.26, 0.24, 0.12, 0.26, -vh,   "compression" }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.18, 0.25, -0.30, "compression" }, /* compression */
    { 0.28, 0.26, 0.24, 0.24, 0.24,  0.00, "extension" }, /* bottom */
    { 0.28, 0.26, 0.24, 0.30, 0.25,  0.30, "extension" }, /* extension */
    { 0.28, 0.26, 0.24, 0.36, 0.26,  vh,   "rising" }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.42, 0.27,  0.30, "rising" }, /* rising */
    { 0.28, 0.26, 0.24, 0.48, 0.28,  0.00, "falling" }, /* apex */
    { 0, 0, 0, 0, 0, 0, "invalid" },
  };
  struct case_t *c;
  char buf[RHC_BUFSIZ];

  for( c=cases; c->za != 0; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    ctrl_update( &ctrl, c->t, p );
    ctrl_phase_string( &ctrl, buf );
  }
}

TEST(test_ctrl_events_at_apex)
{
  double vh = sqrt( 0.04 * G );
  struct case_t{
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expectations */
    struct _ctrl_events_tuple_t apex;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.00, 0.28,  0.00, { 0.00, 0.28,  0.00 } }, /* apex */
    { 0.28, 0.26, 0.24, 0.06, 0.27, -0.30, { 0.00, 0.28,  0.00 } }, /* falling */
    { 0.28, 0.26, 0.24, 0.12, 0.26, -vh,   { 0.00, 0.28,  0.00 } }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.18, 0.25, -0.30, { 0.00, 0.28,  0.00 } }, /* compression */
    { 0.28, 0.26, 0.24, 0.24, 0.24,  0.00, { 0.00, 0.28,  0.00 } }, /* bottom */
    { 0.28, 0.26, 0.24, 0.30, 0.25,  0.30, { 0.00, 0.28,  0.00 } }, /* extension */
    { 0.28, 0.26, 0.24, 0.36, 0.26,  vh,   { 0.36, 0.26,  vh   } }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.42, 0.27,  0.30, { 0.42, 0.27,  0.30 } }, /* rising */
    { 0.28, 0.26, 0.24, 0.48, 0.28,  0.00, { 0.48, 0.28,  0.00 } }, /* apex */
    { 0, 0, 0, 0, 0, 0, { 0.00, 0.00,  0.00 } },
  };
  struct case_t *c;

  for( c=cases; c->za > 0; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    ctrl_update( &ctrl, c->t, p );
    ASSERT_EQ( c->apex.t, ctrl_events_at( &ctrl, apex )->t );
    ASSERT_EQ( c->apex.z, ctrl_events_at( &ctrl, apex )->z );
    ASSERT_EQ( c->apex.v, ctrl_events_at( &ctrl, apex )->v );
  }
}

TEST(test_ctrl_events_at_touchdown)
{
  double vh = sqrt( 0.04 * G );
  struct case_t{
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expectations */
    struct _ctrl_events_tuple_t touchdown;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.00, 0.28,  0.00, { 0.00, 0.28,  0.00 } }, /* apex */
    { 0.28, 0.26, 0.24, 0.06, 0.27, -0.30, { 0.06, 0.27, -0.30 } }, /* falling */
    { 0.28, 0.26, 0.24, 0.12, 0.26, -vh,   { 0.12, 0.26, -vh   } }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.18, 0.25, -0.30, { 0.12, 0.26, -vh   } }, /* compression */
    { 0.28, 0.26, 0.24, 0.24, 0.24,  0.00, { 0.12, 0.26, -vh   } }, /* bottom */
    { 0.28, 0.26, 0.24, 0.30, 0.25,  0.30, { 0.12, 0.26, -vh   } }, /* extension */
    { 0.28, 0.26, 0.24, 0.36, 0.26,  vh,   { 0.12, 0.26, -vh   } }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.42, 0.27,  0.30, { 0.12, 0.26, -vh   } }, /* rising */
    { 0.28, 0.26, 0.24, 0.48, 0.28,  0.00, { 0.48, 0.28,  0.00 } }, /* apex */
    { 0, 0, 0, 0, 0, 0, { 0.00, 0.00,  0.00 } },
  };
  struct case_t *c;

  for( c=cases; c->za > 0; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    ctrl_update( &ctrl, c->t, p );
    ASSERT_EQ( c->touchdown.t, ctrl_events_at( &ctrl, touchdown )->t );
    ASSERT_EQ( c->touchdown.z, ctrl_events_at( &ctrl, touchdown )->z );
    ASSERT_EQ( c->touchdown.v, ctrl_events_at( &ctrl, touchdown )->v );
  }
}

TEST(test_ctrl_events_at_bottom)
{
  double vh = sqrt( 0.04 * G );
  struct case_t{
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expectations */
    struct _ctrl_events_tuple_t bottom;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.00, 0.28,  0.00, { 0.00, 0.00,  0.00 } }, /* apex */
    { 0.28, 0.26, 0.24, 0.06, 0.27, -0.30, { 0.00, 0.00,  0.00 } }, /* falling */
    { 0.28, 0.26, 0.24, 0.12, 0.26, -vh,   { 0.12, 0.26, -vh   } }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.18, 0.25, -0.30, { 0.18, 0.25, -0.30 } }, /* compression */
    { 0.28, 0.26, 0.24, 0.24, 0.24,  0.00, { 0.24, 0.24,  0.00 } }, /* bottom */
    { 0.28, 0.26, 0.24, 0.30, 0.25,  0.30, { 0.24, 0.24,  0.00 } }, /* extension */
    { 0.28, 0.26, 0.24, 0.36, 0.26,  vh,   { 0.24, 0.24,  0.00 } }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.42, 0.27,  0.30, { 0.24, 0.24,  0.00 } }, /* rising */
    { 0.28, 0.26, 0.24, 0.48, 0.28,  0.00, { 0.24, 0.24,  0.00 } }, /* apex */
    { 0, 0, 0, 0, 0, 0, { 0.00, 0.00,  0.00 } },
  };
  struct case_t *c;

  for( c=cases; c->za > 0; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    ctrl_update( &ctrl, c->t, p );
    ASSERT_EQ( c->bottom.t, ctrl_events_at( &ctrl, bottom )->t );
    ASSERT_EQ( c->bottom.z, ctrl_events_at( &ctrl, bottom )->z );
    ASSERT_EQ( c->bottom.v, ctrl_events_at( &ctrl, bottom )->v );
  }
}

TEST(test_ctrl_events_at_liftoff)
{
  double vh = sqrt( 0.04 * G );
  struct case_t{
    /* parameters */
    double za, zh, zb;
    /* states */
    double t, z, v;
    /* expectations */
    struct _ctrl_events_tuple_t liftoff;
  } cases[] = {
    { 0.28, 0.26, 0.24, 0.00, 0.28,  0.00, { 0.00, 0.00,  0.00 } }, /* apex */
    { 0.28, 0.26, 0.24, 0.06, 0.27, -0.30, { 0.00, 0.00,  0.00 } }, /* falling */
    { 0.28, 0.26, 0.24, 0.12, 0.26, -vh,   { 0.00, 0.00,  0.00 } }, /* touchdown */
    { 0.28, 0.26, 0.24, 0.18, 0.25, -0.30, { 0.00, 0.00,  0.00 } }, /* compression */
    { 0.28, 0.26, 0.24, 0.24, 0.24,  0.00, { 0.24, 0.24,  0.00 } }, /* bottom */
    { 0.28, 0.26, 0.24, 0.30, 0.25,  0.30, { 0.30, 0.25,  0.30 } }, /* extension */
    { 0.28, 0.26, 0.24, 0.36, 0.26,  vh,   { 0.36, 0.26,  vh   } }, /* lift-off */
    { 0.28, 0.26, 0.24, 0.42, 0.27,  0.30, { 0.36, 0.26,  vh   } }, /* rising */
    { 0.28, 0.26, 0.24, 0.48, 0.28,  0.00, { 0.36, 0.26,  vh   } }, /* apex */
    { 0, 0, 0, 0, 0, 0, { 0.00, 0.00,  0.00 } },
  };
  struct case_t *c;

  for( c=cases; c->za > 0; c++ ){
    vec_set_elem_list( p, c->z, c->v );
    ctrl_update( &ctrl, c->t, p );
    ASSERT_EQ( c->liftoff.t, ctrl_events_at( &ctrl, liftoff )->t );
    ASSERT_EQ( c->liftoff.z, ctrl_events_at( &ctrl, liftoff )->z );
    ASSERT_EQ( c->liftoff.v, ctrl_events_at( &ctrl, liftoff )->v );
  }
}

TEST_SUITE(test_ctrl)
{
  CONFIGURE_SUITE( setup, teardown );
  RUN_TEST(test_ctrl_init);
  RUN_TEST(test_ctrl_destroy);
  RUN_TEST(test_ctrl_za);
  RUN_TEST(test_ctrl_zh);
  RUN_TEST(test_ctrl_zm);
  RUN_TEST(test_ctrl_zb);
  RUN_TEST(test_ctrl_vh);
  RUN_TEST(test_ctrl_reset_default);
  RUN_TEST(test_ctrl_update_default);
  RUN_TEST(test_ctrl_phase);
  RUN_TEST(test_ctrl_phase_string);
  RUN_TEST(test_ctrl_events_at_apex);
  RUN_TEST(test_ctrl_events_at_touchdown);
  RUN_TEST(test_ctrl_events_at_bottom);
  RUN_TEST(test_ctrl_events_at_liftoff);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_ctrl_events);
  RUN_SUITE(test_ctrl);
  TEST_REPORT();
  TEST_EXIT();
}
