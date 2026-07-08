#include "rhc_ctrl_dynmorph.h"

double _ctrl_dynmorph_calc_phi_ratio_linear(double phi);
double _ctrl_dynmorph_calc_phi_ratio_piecewise(double phi);

cmd_t *ctrl_dynmorph_cmd_init(ctrl_t *self, cmd_t *cmd)
{
  cmd_default_init( cmd );
  ctrl_dynmorph_set_rho( self, 0.0 );
  ctrl_dynmorph_set_k( self, 4.0 );
  ctrl_dynmorph_set_q_scale( self, 1.0 );
  ctrl_dynmorph_enable_soft_landing( self );
  return cmd;
}

ctrl_t *ctrl_dynmorph_create_with_type(ctrl_t *self, cmd_t *cmd, model_t *model, enum ctrl_dynmorph_types type)
{
  ctrl_dynmorph_prp *prp;

  ctrl_init( self, cmd, model );
  ctrl_dynmorph_cmd_init( self, cmd );
  self->_reset = ctrl_dynmorph_reset;
  self->_update = ctrl_dynmorph_update;
  self->_destroy = ctrl_dynmorph_destroy;
  self->_header = ctrl_dynmorph_header;
  self->_writer = ctrl_dynmorph_writer;

  if( ( self->prp = nalloc( ctrl_dynmorph_prp, 1 ) ) == NULL ){
    ALLOC_ERR();
    return NULL;
  }
  prp = self->prp;
  prp->type = type;

  if( prp->type == fix_zb ){
    prp->_update_params = ctrl_dynmorph_update_params_fix_zb;
    prp->_calc_phi_ratio = _ctrl_dynmorph_calc_phi_ratio_piecewise;
  } else if( prp->type == fix_zm ){
    prp->_update_params = ctrl_dynmorph_update_params_fix_zm;
    prp->_calc_phi_ratio = _ctrl_dynmorph_calc_phi_ratio_piecewise;
  } else{
    prp->_update_params = ctrl_dynmorph_update_params_default;
    if( prp->type == soft_landing_linear ){
      prp->_calc_phi_ratio = _ctrl_dynmorph_calc_phi_ratio_linear;
    } else{
      prp->_calc_phi_ratio = _ctrl_dynmorph_calc_phi_ratio_piecewise;
    }
  }
  cmd_copy( cmd, ctrl_dynmorph_params(self) );
  prp->q1 = 0;
  prp->q2 = 0;
  prp->vm = 0;
  prp->cushioning = false;
  return self;
}

ctrl_t *ctrl_dynmorph_reset(ctrl_t *self, void *util)
{
  ctrl_reset_default( self, util );
  ctrl_dynmorph_cushioning(self) = false;
  return self;
}

void ctrl_dynmorph_destroy(ctrl_t *self)
{
  sfree( self->prp );
  ctrl_destroy_default( self );
}

void ctrl_dynmorph_header(FILE *fp, void *util)
{
  fprintf( fp, ",type,rho,k,q_scale,soft_landing,q1,q2,vm,p_za,p_zh,p_zm,p_zb,p_rho" );
}

void ctrl_dynmorph_writer(FILE *fp, ctrl_t *self, void *util)
{
  fprintf( fp, ",%d,%f,%f,%f,%d,%f,%f,%f,%f,%f,%f,%f,%f",
           ctrl_dynmorph_type(self),
           ctrl_dynmorph_rho(self),
           ctrl_dynmorph_k(self),
           ctrl_dynmorph_q_scale(self),
           ctrl_dynmorph_soft_landing(self),
           ctrl_dynmorph_q1(self),
           ctrl_dynmorph_q2(self),
           ctrl_dynmorph_vm(self),
           ctrl_dynmorph_params_za(self),
           ctrl_dynmorph_params_zh(self),
           ctrl_dynmorph_params_zm(self),
           ctrl_dynmorph_params_zb(self),
           ctrl_dynmorph_params_rho(self) );
 }

double ctrl_dynmorph_calc_q1(double zh, double zm, double g)
{
  return sqrt( g / ( zh - zm ) );
}

double ctrl_dynmorph_calc_r(double zm, double zb)
{
  double r;

  r = zm - zb;
  if( istiny(r) )
    RUNTIME_WARN( "rhc_ctrl_dynmorph: zm and zb are too close!" );
  return r;
}

double ctrl_dynmorph_calc_sqr_vm(double zh, double zm, double zb, double g)
{
  double r;

  r = ctrl_dynmorph_calc_r( zm, zb );
  return g * r * r / ( zh - zm );
}

double ctrl_dynmorph_calc_sqr_gamma(vec_t p, double zh, double zm, double zb, double g)
{
  double z, v, r, sqr_r, sqr_vm;

  z = vec_elem( p, 0 );
  v = vec_elem( p, 1 );
  r = ctrl_dynmorph_calc_r( zm, zb );
  sqr_r = r * r;
  sqr_vm = ctrl_dynmorph_calc_sqr_vm( zh, zm, zb, g );
  return ( z - zm ) * ( z - zm ) / sqr_r + v * v / sqr_vm;
}

/* Radius (in gamma) of the stance-phase limit cycle, i.e. the root of
   the nonlinear damping term h(gamma) = 1 - rho * exp( k ( 1 - gamma ) ).
   A non-positive value means the equilibrium (zm, 0) is stable and no
   limit cycle exists; the cycle appears when rho > exp( -k ). */
double ctrl_dynmorph_calc_gamma_lc(double rho, double k)
{
  if( rho <= 0.0 || k <= 0.0 )
    return -HUGE_VAL;
  return 1.0 + log( rho ) / k;
}

double ctrl_dynmorph_calc_za(double zh, double zm, double zb)
{
  return 0.5 * ( ( zm - zb ) * ( zm - zb ) / ( zh - zm ) + zh + zm );
}

double ctrl_dynmorph_calc_zh(double za, double zm, double zb)
{
  return za - sqrt( ( za - zb ) * ( za + zb - 2.0 * zm ) );
}

double ctrl_dynmorph_calc_zm(double za, double zh, double zb)
{
  if( za > zh ){
    return 0.5 * ( ( za + zb - ( za - zh ) * ( za - zh ) / ( za - zb ) ) );
  } else{
    return 0.5 * ( za + zb );
  }
}

double ctrl_dynmorph_calc_zb(double za, double zh, double zm)
{
  return zm - sqrt( ( zh - zm ) * ( 2.0 * za - zh - zm ) );
}

double _ctrl_dynmorph_calc_phi_ratio_linear(double phi)
{
  double s;
  if( phi < 0.0 ) s = 1.0;
  else{
    s = (2.0 * phi) / PI - 1.0;
    if( s < 0.0 ) s = 0.0;
    else if( s > 1.0 ) s = 1.0;
  }
  return s;
}

double _ctrl_dynmorph_calc_phi_ratio_piecewise(double phi)
{
  double s;
  if( phi < -PI_2 ) s = 1.0;
  else s = 0.0;
  return s;
}

double ctrl_dynmorph_calc_phi_based_param(ctrl_t *self, double phi, double v1, double v2)
{
  double s, v;

  s = ctrl_dynmorph_get_prp(self)->_calc_phi_ratio( phi );
  v = v1 * ( 1.0 - s ) + v2 * s;
  return v;
}

void ctrl_dynmorph_print_params(FILE *fp, ctrl_t *self, vec_t p, const char *sep)
{
  char s[RHC_BUFSIZ];

  if( sep ){
    fprintf( fp, "%s\n", sep );
  }
  ctrl_events_get_phase_string(ctrl_phase(self), s);
  fprintf( fp, "phase: %s, phi: %f, z: %f, vz: %f\n", s, ctrl_phi(self), vec_elem(p,0), vec_elem(p,1) );
  fprintf( fp, "  za: %f, zh: %f, zm: %f, zb: %f, rho: %f, k: %f, soft_landing: %s, cushioning: %s\n",
           ctrl_za(self),
           ctrl_zh(self),
           ctrl_zm(self),
           ctrl_zb(self),
           ctrl_dynmorph_rho(self),
           ctrl_dynmorph_k(self),
           ctrl_dynmorph_soft_landing(self) ? "true" : "false",
           ctrl_dynmorph_cushioning(self) ? "true" : "false" );
  fprintf( fp, "  za: %f, zh: %f, zm: %f, zb: %f, rho: %f\n",
           ctrl_dynmorph_params_za(self),
           ctrl_dynmorph_params_zh(self),
           ctrl_dynmorph_params_zm(self),
           ctrl_dynmorph_params_zb(self),
           ctrl_dynmorph_params_rho(self) );
}

ctrl_t *ctrl_dynmorph_update_params_default(ctrl_t *self, vec_t p)
{
  cmd_t *params;
  double za, zb, zm, rho;
  double z_apex;

  params = ctrl_dynmorph_params(self);
  cmd_copy( ctrl_cmd(self), params );

  if( ctrl_phase_in( self, flight ) ){
    /* Once the robot goes into flight, cushioning is enabled */
    ctrl_dynmorph_cushioning(self) = true;
  } else if( ctrl_phase_in( self, extension ) ){
    /* Once the robot extends its body, cushioning is no more needed */
    ctrl_dynmorph_cushioning(self) = false;
  }

  /* When soft landing is enabled, fix desired apex and rho if needed */
  if( ctrl_dynmorph_soft_landing(self) ){
    if( istiny( ctrl_apex_z(self) ) || !ctrl_dynmorph_cushioning(self) ){
      /* Use desired apex height is used as the robot goes into flight yet */
      z_apex = ctrl_za(self);
    } else{
      z_apex = max(ctrl_apex_z(self), ctrl_za(self));
    }
    /* Fix the apex height to absorb the impact when touching down */
    za = ctrl_dynmorph_calc_phi_based_param( self, ctrl_phi(self), z_apex, ctrl_za(self) );
    params->za = max(za, ctrl_za(self));
    if( istiny( ctrl_dynmorph_rho(self) ) && ctrl_dynmorph_cushioning(self) ){
      /* Fix rho when the robot is in flight phase but it wants to stand */
      rho = ctrl_dynmorph_calc_phi_based_param( self, ctrl_phi(self), 1.0, ctrl_dynmorph_rho(self) );
      params->dynmorph.rho = rho;
    }
  }

  /* When hopping or squatting, the crouching height is fixed so that
     it does not fall behind the kinematic lower limit */
  if( params->dynmorph.rho > 0 ){
    zb = ctrl_dynmorph_calc_zb( params->za, ctrl_zh(self), ctrl_zm(self) );
    if( ctrl_zb(self) < zb && ctrl_za(self) > ctrl_zh(self) ){
      /* The couching height will be over the kinematic limit */
      params->zb = zb;
    } else{
      /* The couching height falls behind the kinematic limit or squatting */
      zm = ctrl_dynmorph_calc_zm( params->za, ctrl_zh(self), ctrl_zb(self) );
      params->zm = zm;
    }
  }
  return self;
}

ctrl_t *ctrl_dynmorph_update_params_fix_zb(ctrl_t *self, vec_t p)
{
  cmd_t *params;
  double zm;

  params = ctrl_dynmorph_params(self);
  cmd_copy( ctrl_cmd(self), params );
  zm = ctrl_dynmorph_calc_zm( ctrl_za(self), ctrl_zh(self), ctrl_zb(self) );
  params->zm = zm;
  return self;
}

ctrl_t *ctrl_dynmorph_update_params_fix_zm(ctrl_t *self, vec_t p)
{
  cmd_t *params;
  double zb;

  params = ctrl_dynmorph_params(self);
  cmd_copy( ctrl_cmd(self), params );
  zb = ctrl_dynmorph_calc_zb( ctrl_za(self), ctrl_zh(self), ctrl_zm(self) );
  params->zb = zb;
  return self;
}

ctrl_t *ctrl_dynmorph_update(ctrl_t *self, double t, vec_t p)
{
  ctrl_update_default( self, t, p );
  ctrl_dynmorph_get_prp(self)->_update_params( self, p );
  if( ctrl_phase_in( self, flight ) ){
    self->fz = 0;
  } else{
    self->fz = ctrl_dynmorph_calc_fz( self, p );
  }
  if( self->fz < 0 ) self->fz = 0;
  return self;
}

double ctrl_dynmorph_calc_fz(ctrl_t *self, vec_t p)
{
  double zh, zm, zb, rho, k, q_scale;
  double g, k1, q1, r, vm, gamma, f_gamma, unit_fz;
  ctrl_dynmorph_prp *prp;

  prp = ctrl_dynmorph_get_prp(self);
  zh = ctrl_dynmorph_params_zh( self );
  zm = ctrl_dynmorph_params_zm( self );
  zb = ctrl_dynmorph_params_zb( self );
  rho = ctrl_dynmorph_params_rho( self );
  k = ctrl_dynmorph_k( self );
  q_scale = ctrl_dynmorph_q_scale( self );
  g = model_gravity( ctrl_model(self) );
  q1 = ctrl_dynmorph_calc_q1( zh, zm, g ) * q_scale;
  k1 = q1 * q1;
  prp->q1 = prp->q2 = q1;
  r = ctrl_dynmorph_calc_r( zm, zb );
  vm = q1 * r;
  prp->vm = vm;
  gamma = sqrt( sqr( vec_elem(p,0) - zm ) / sqr( r ) + sqr( vec_elem(p,1) ) / sqr( vm ) );
  f_gamma = 1.0 - rho * exp( k * ( 1.0 - gamma ) );
  unit_fz = -2.0 * q1 * f_gamma * vec_elem(p,1) - k1 * ( vec_elem(p,0) - zm ) + g;
  return model_mass( ctrl_model( self ) ) * unit_fz;
}
