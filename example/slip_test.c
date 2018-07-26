#include "rhc_simulator.h"
#include "rhc_ctrl_slip.h"

#define DT 0.001
#define T  10

void header(FILE *fp, void *util) {
  fprintf(fp, "t,z,v,fz,fe,zd,z0,zb\n" );
}

void output(FILE *fp, simulator_t *s, void *util) {
  vec_t state = simulator_state(s);
  ctrl_t *ctrl = simulator_ctrl(s);
  cmd_t *cmd = simulator_cmd(s);
  fprintf( fp, "%f,%f,%f,%f,%f,%f,%f,%f\n",
           simulator_time(s),
           vec_elem(state, 0), vec_elem(state, 1),
           ctrl_fz(ctrl), simulator_fe(s),
           cmd->zd, cmd->z0, cmd->zb );
}

int main(int argc, char *argv[])
{
  cmd_t cmd;
  model_t model;
  ctrl_t ctrl;
  logger_t logger;
  simulator_t sim;
  vec_t p0;
  double z0;

  cmd_default_init( &cmd );
  model_init( &model, 10 );
  ctrl_slip_create( &ctrl, &cmd, &model );
  logger_init( &logger );
  simulator_init( &sim, &cmd, &ctrl, &model );
  simulator_set_default_logger( &sim, &logger );
  p0 = vec_create( 2 );

  for( z0 = cmd.z0 - 0.005; z0 < cmd.zd + 0.1; z0 += 0.005 ) {
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
