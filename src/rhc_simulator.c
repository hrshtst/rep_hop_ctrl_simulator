#include "rhc_simulator.h"

simulator_t *simulator_init(simulator_t *self, cmd_t *cmd, ctrl_t *ctrl, model_t *model)
{
  simulator_cmd( self )   = cmd;
  simulator_ctrl( self )  = ctrl;
  simulator_model( self ) = model;
  simulator_time( self )  = 0;
  simulator_step( self )  = 0;
  simulator_state( self ) = vec_create( 2 );
  simulator_set_fe( self, 0 );
  ode_assign( &self->ode, rk4 );
  ode_init( &self->ode, 2, simulator_dp );
  self->n_trial = 0;
  simulator_set_reset_fp( self, simulator_reset_ctrl );
  simulator_set_update_fp( self, simulator_update );
  simulator_set_dump_fp( self, simulator_dump );
  simulator_set_tag( self, "" );
  return self;
}

void simulator_destroy(simulator_t *self)
{
  simulator_cmd( self )   = NULL;
  simulator_ctrl( self )  = NULL;
  simulator_model( self ) = NULL;
  simulator_time( self )  = 0;
  simulator_step( self )  = 0;
  vec_destroy( simulator_state( self ) );
  simulator_state( self ) = NULL;
  simulator_set_fe( self, 0 );
  if( self->ode._ws )
    ode_destroy( &self->ode );
  self->n_trial = 0;
  simulator_set_tag( self, "" );
}

void simulator_set_reset_fp(simulator_t *self, bool (*reset_fp)(simulator_t*, void*))
{
  self->reset_fp = reset_fp;
}

void simulator_set_update_fp(simulator_t *self, bool (*update_fp)(simulator_t*, double, void*))
{
  self->update_fp = update_fp;
}

void simulator_set_dump_fp(simulator_t *self, void (*dump_fp)(simulator_t*, logger_t*, void*))
{
  self->dump_fp = dump_fp;
}

char *simulator_set_tag(simulator_t *self, const char* tag)
{
  string_copy( tag, self->tag );
  return self->tag;
}

char *simulator_update_default_tag(simulator_t *self) {
  char default_tag[BUFSIZ];

  snprintf( default_tag, sizeof(default_tag), "sc%05d", simulator_n_trial(self) );
  return simulator_set_tag( self, default_tag );
}

bool simulator_has_default_tag(simulator_t *self) {
  int len;

  len = string_len( simulator_tag(self) );
  if( len == 0 ) return true;
  else if( len < 5 ) return false;

  const char *c = &simulator_tag(self)[len-5];
  if( string_is_digit( c ) && atoi( c ) == simulator_n_trial(self) - 1 )
    return true;
  return false;
}

vec_t simulator_dp(double t, vec_t x, void *util, vec_t v)
{
  simulator_t *sim = util;

  ctrl_update( sim->ctrl, t, x );
  model_update( sim->model, ctrl_fz(sim->ctrl), simulator_fe(sim) );
  vec_set_elem( v, 0, vec_elem(x,1) );
  vec_set_elem( v, 1, model_acc( sim->model ) );
  return v;
}

bool simulator_reset_ctrl(simulator_t *self, void *util)
{
  if( ctrl_reset( simulator_ctrl(self), util ) )
    return true;
  else
    return false;
}

bool simulator_reset(simulator_t *self, void *util)
{
  simulator_time( self ) = 0;
  simulator_step( self ) = 0;
  if( self->reset_fp ){
    if( !self->reset_fp( self, util ) ){
      RUNTIME_ERR( ERR_RESET_FAIL );
      return false;
    }
  }
  return true;
}

bool simulator_update(simulator_t *self, double dt, void *util)
{
  ode_update( &self->ode, simulator_time(self), simulator_state(self), dt, self );
  return true;
}

void simulator_update_time(simulator_t *self, double dt)
{
  simulator_inc_step( self );
  simulator_time(self) = simulator_step(self) * dt;
}

void simulator_run(simulator_t *self, vec_t p0, double time, double dt, logger_t *logger, void *util)
{
  if( !simulator_reset( self, util ) ) return;
  simulator_set_state( self, p0 );
  if( simulator_has_default_tag( self ) ){
    simulator_update_default_tag( self );
  }
  if( logger && !logger_is_open(logger) )
    RUNTIME_WARN( "Logger is not opened" );
  while( simulator_time(self) < time ){
    self->dump_fp( self, logger, util );
    if( !self->update_fp( self, dt, util ) )
      break;
    simulator_update_time( self, dt );
  }
  simulator_inc_trial( self );
}

void simulator_header_default(FILE *fp, simulator_t *s, void *util)
{
  /* simulator, states, model */
  fprintf( fp, "tag,t,z,vz,m,az,fe" );
  /* controller */
  fprintf( fp, ",fz,za,zh,zm,zb,vh,n,phi" );
  /* phase */
  fprintf( fp, ",phase" );
  /* events */
  fprintf( fp, ",ap_t,ap_z,ap_v" );
  fprintf( fp, ",td_t,td_z,td_v" );
  fprintf( fp, ",bt_t,bt_z,bt_v" );
  fprintf( fp, ",lo_t,lo_z,lo_v" );
  ctrl_header( fp, s->ctrl, util );
}

void simulator_writer_default(FILE *fp, simulator_t *s, void *util)
{
  vec_t state = simulator_state(s);
  model_t *model = simulator_model(s);
  ctrl_t *ctrl = simulator_ctrl(s);
  /* simulator, states, model */
  fprintf( fp, "%s,%f,%f,%f,%f,%f,%f",
           simulator_tag(s), simulator_time(s),
           vec_elem(state,0), vec_elem(state,1),
           model_mass(model), model_acc(model),
           simulator_fe(s) );
  /* controller */
  fprintf( fp, ",%f,%f,%f,%f,%f,%f,%d,%f",
           ctrl_fz(ctrl),
           ctrl_za(ctrl), ctrl_zh(ctrl), ctrl_zm(ctrl), ctrl_zb(ctrl),
           ctrl_vh(ctrl),
           ctrl_n(ctrl), ctrl_phi(ctrl));
  /* phase and events */
  fprintf( fp, ",%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
           ctrl_phase(ctrl),
           ctrl_events_at(ctrl,apex)->t,
           ctrl_events_at(ctrl,apex)->z,
           ctrl_events_at(ctrl,apex)->v,
           ctrl_events_at(ctrl,touchdown)->t,
           ctrl_events_at(ctrl,touchdown)->z,
           ctrl_events_at(ctrl,touchdown)->v,
           ctrl_events_at(ctrl,bottom)->t,
           ctrl_events_at(ctrl,bottom)->z,
           ctrl_events_at(ctrl,bottom)->v,
           ctrl_events_at(ctrl,liftoff)->t,
           ctrl_events_at(ctrl,liftoff)->z,
           ctrl_events_at(ctrl,liftoff)->v );
  ctrl_writer( fp, s->ctrl, util );
}

void simulator_set_default_logger(simulator_t *self, logger_t *logger)
{
  logger_register( logger, simulator_header_default, simulator_writer_default );
}

void simulator_dump(simulator_t *self, logger_t *logger, void *util)
{
  if( logger )
    logger_write( logger, self, util );
}

void simulator_dump_header(simulator_t *self, logger_t *logger, void *util)
{
  logger_write_header( logger, self, NULL );
}

void simulator_dump_data(simulator_t *self, logger_t *logger, void *util)
{
  logger_write_data( logger, self, util );
}
