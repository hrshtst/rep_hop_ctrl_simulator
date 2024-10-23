#include "rhc_ctrl_arl.h"
#include "rhc_simulator.h"

#define DT 0.001
#define T  10


int main(int argc, char *argv[])
{
  cmd_t cmd;
  model_t model;
  ctrl_t ctrl;
  logger_t logger;
  simulator_t sim;
  vec_t p0;
  double z0;
  vec_t z0_list;
  register int i;

  cmd_default_init( &cmd );
  model_init( &model, 15 );
  ctrl_arl_create( &ctrl, &cmd, &model, ahmadi2007 );
  ctrl_arl_set_params( &ctrl, 10000, 200000 );
  logger_init( &logger );
  simulator_init( &sim, &cmd, &ctrl, &model );
  simulator_set_default_logger( &sim, &logger );
  p0 = vec_create( 2 );
  z0_list = vec_create_list( 3, 0.275, 0.28, 0.285 );

  for( i=0; i<vec_size( z0_list ); i++ ){
    z0 = vec_elem(z0_list, i);
    vec_set_elem_list( p0, z0, 0.0 );
    simulator_run( &sim, p0, T, DT, &logger, NULL );
  }

  vec_destroy( p0 );
  simulator_destroy( &sim );
  logger_destroy( &logger );
  ctrl_destroy( &ctrl );
  model_destroy( &model );
  cmd_destroy( &cmd );
  return 0;
}