#include "rhc_ctrl.h"
#include "rhc_misc.h"
#include "rhc_string.h"

static enum _ctrl_events_phases_t _ctrl_events_determine_phase(double phi);
static void _ctrl_events_update_event(ctrl_events_t *self, enum _ctrl_events_phases_t phase, double t, vec_t p, cmd_t *cmd);

ctrl_events_t *ctrl_events_init(ctrl_events_t *self)
{
  ctrl_events_apex(self)->t = 0;
  ctrl_events_apex(self)->z = 0;
  ctrl_events_apex(self)->v = 0;
  ctrl_events_touchdown(self)->t = 0;
  ctrl_events_touchdown(self)->z = 0;
  ctrl_events_touchdown(self)->v = 0;
  ctrl_events_bottom(self)->t = 0;
  ctrl_events_bottom(self)->z = 0;
  ctrl_events_bottom(self)->v = 0;
  ctrl_events_liftoff(self)->t = 0;
  ctrl_events_liftoff(self)->z = 0;
  ctrl_events_liftoff(self)->v = 0;
  self->phase = invalid;
  ctrl_events_phi(self) = 0;
  ctrl_events_n(self) = 0;
  self->is_updated = false;
  return self;
}

void ctrl_events_destroy(ctrl_events_t *self)
{
  ctrl_events_init( self );
}

char *ctrl_events_get_phase_string(enum _ctrl_events_phases_t phase, char *s)
{
  switch( phase ){
  case falling:
    string_copy( "falling", s );
    break;
  case compression:
    string_copy( "compression", s );
    break;
  case extension:
    string_copy( "extension", s );
    break;
  case rising:
    string_copy( "rising", s );
    break;
  default:
    string_copy( "invalid", s );
    break;
  }
  return s;
}

complex_t *ctrl_events_calc_phase_complex(double zh, double za, double zb, vec_t p, double g, complex_t *c)
{
  double z, v, vh;

  z = vec_elem( p, 0 );
  v = vec_elem( p, 1 );
  vh = ctrl_calc_vh( zh, za, g );
  complex_init( c, (z-zh)/(zh-zb), -v/vh );
  return c;
}

double ctrl_events_calc_phi(double zh, double za, double zb, vec_t p, double g)
{
  complex_t c;

  ctrl_events_calc_phase_complex( zh, za, zb, p, g, &c );
  return complex_arg( &c );
}

bool ctrl_events_is_in_falling(ctrl_events_t *self)
{
  return ctrl_events_phase(self) == falling;
}

bool ctrl_events_is_in_compression(ctrl_events_t *self)
{
  return ctrl_events_phase(self) == compression;
}

bool ctrl_events_is_in_extension(ctrl_events_t *self)
{
  return ctrl_events_phase(self) == extension;
}

bool ctrl_events_is_in_rising(ctrl_events_t *self)
{
  return ctrl_events_phase(self) == rising;
}

bool ctrl_events_is_in_flight(ctrl_events_t *self)
{
  /* Flight phase is the union of rising and falling phases. */
  return ctrl_events_is_in_rising( self ) || ctrl_events_is_in_falling( self );
}

bool ctrl_events_is_in_contact(ctrl_events_t *self)
{
  /* Contact (stance) phase is the union of compression and extension phases. */
  return ctrl_events_is_in_compression( self ) || ctrl_events_is_in_extension( self );
}

enum _ctrl_events_phases_t _ctrl_events_determine_phase(double phi)
{
  if( ( 0 <= phi ) && ( phi < PI_2 ) ){
    /* Falling phase includes apex event, but not touchdown. */
    return falling;
  } else if( ( PI_2 <= phi ) && ( phi < PI ) ){
    /* Compression phase includes touchdown event, but not bottom. */
    return compression;
  } else if( ( -PI <= phi ) && ( phi < -PI_2 ) ){
    /* Extension phase includes bottom event, but not lift-off. */
    return extension;
  } else if( ( -PI_2 <= phi ) && ( phi < 0 ) ){
    /* Rising phase includes lift-off event, but not apex. */
    return rising;
  } else {
    return invalid;
  }
}

void _ctrl_events_update_event(ctrl_events_t *self, enum _ctrl_events_phases_t phase, double t, vec_t p, cmd_t *cmd)
{
  double z, v;

  z = vec_elem( p, 0 );
  v = vec_elem( p, 1 );

  /* Initially we need to handle each event in exceptional way. */
  if( ctrl_events_phase(self) == invalid ){
    switch( phase ){
      case falling:
        ctrl_events_set_apex( self, t, z, v );
        break;
      case compression:
        ctrl_events_set_touchdown( self, t, z, v );
        break;
      case extension:
        ctrl_events_set_bottom( self, t, z, v );
        break;
      case rising:
        ctrl_events_set_liftoff( self, t, z, v );
        break;
      default:
        break;
    }
  }

  if( phase == rising || ctrl_events_is_in_rising( self ) ){
    if( ctrl_events_is_in_extension( self ) ||
        z >= ctrl_events_apex(self)->z ){
      ctrl_events_set_apex( self, t, z, v );
    }
  }
  if( phase == falling || ctrl_events_is_in_falling( self ) ){
    if( ctrl_events_is_in_rising( self ) ||
        v <= ctrl_events_touchdown(self)->v ){
      ctrl_events_set_touchdown( self, t, z, v );
    }
  }
  if( phase == compression || ctrl_events_is_in_compression( self ) ){
    if( ctrl_events_is_in_falling( self ) ||
        z <= ctrl_events_bottom(self)->z ){
      ctrl_events_set_bottom( self, t, z, v );
    }
  }
  if( phase == extension || ctrl_events_is_in_extension( self ) ){
    if( ctrl_events_is_in_compression( self ) ||
        v >= ctrl_events_liftoff(self)->v ){
      ctrl_events_set_liftoff( self, t, z, v );
    }
  }
}

ctrl_events_t *ctrl_events_update(ctrl_events_t *self, double t, vec_t p, cmd_t *cmd, double g)
{
  double phi;
  enum _ctrl_events_phases_t phase;

  if( ctrl_events_is_updated(self) ) return self;
  phi = ctrl_events_calc_phi( cmd->zh, cmd->za, cmd->zb, p, g );
  if( ctrl_events_phi( self ) < 0 && phi >= 0 ) ctrl_events_n(self)++;
  phase = _ctrl_events_determine_phase( phi );
  _ctrl_events_update_event( self, phase, t, p, cmd );
  ctrl_events_phi(self) = phi;
  ctrl_events_phase(self) = phase;
  ctrl_events_is_updated(self) = true;
  return self;
}

ctrl_t *ctrl_init(ctrl_t *self, cmd_t *cmd, model_t *model)
{
  self->cmd = cmd;
  self->model = model;
  self->fz = 0;
  self->_reset = ctrl_reset_default;
  self->_update = ctrl_update_default;
  self->_destroy = ctrl_destroy_default;
  self->_header = ctrl_header_default;
  self->_writer = ctrl_writer_default;
  self->prp = NULL;
  ctrl_events_init( ctrl_events( self ) );
  return self;
}

void ctrl_destroy_default(ctrl_t *self)
{
  self->cmd = NULL;
  self->model = NULL;
  self->prp = NULL;
  ctrl_events_destroy( ctrl_events( self ) );
}

double ctrl_calc_sqr_vh(double zh, double za, double g)
{
  double sqr_vh;

  sqr_vh = 2.0 * g * ( za - zh );
  return fabs(sqr_vh);
}

ctrl_t *ctrl_reset_default(ctrl_t *self, void *util)
{
  self->fz = 0;
  ctrl_events_init( ctrl_events( self ) );
  return self;
}

ctrl_t *ctrl_update_default(ctrl_t *self, double t, vec_t p)
{
  ctrl_events_update_next( ctrl_events( self ) );
  ctrl_events_update( ctrl_events( self ), t, p, ctrl_cmd( self ), model_gravity( ctrl_model(self) ) );
  return self;
}

void ctrl_header_default(FILE *fp, void *util)
{}

void ctrl_writer_default(FILE *fp, ctrl_t *self, void *util)
{}
