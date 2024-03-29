#ifndef __RHC_CTRL_RAIBERT_H__
#define __RHC_CTRL_RAIBERT_H__

#include "rhc_ctrl.h"

enum ctrl_raibert_types {
  none = -1,
  full_nonlinear = 0,
  simplified_nonlinear,
  full_linear,
  simplified_linear,
};

typedef struct{
  enum ctrl_raibert_types type;  /* controller type */
  struct _ctrl_events_tuple_t end_of_thrust;
  bool is_in_thrust;
} ctrl_raibert_prp;

ctrl_t *ctrl_raibert_create(ctrl_t *self, cmd_t *cmd, model_t *model, enum ctrl_raibert_types type);
void ctrl_raibert_destroy(ctrl_t *self);
ctrl_t *ctrl_raibert_update(ctrl_t *self, double t, vec_t p);
void ctrl_raibert_header(FILE *fp, void *util);
void ctrl_raibert_writer(FILE *fp, ctrl_t *self, void *util);

void ctrl_raibert_update_events(ctrl_t *self, double t, vec_t p);

#define ctrl_raibert_delta(self)   ( ctrl_cmd(self)->raibert.delta )
#define ctrl_raibert_tau(self)     ( ctrl_cmd(self)->raibert.tau )
#define ctrl_raibert_gamma(self)   ( ctrl_cmd(self)->raibert.gamma )
#define ctrl_raibert_yeta1(self)   ( ctrl_cmd(self)->raibert.yeta1 )
#define ctrl_raibert_zr(self)      ( ctrl_cmd(self)->raibert.zr )
#define ctrl_raibert_mu(self)      ( ctrl_cmd(self)->raibert.mu )
#define ctrl_raibert_get_prp(self) ( (ctrl_raibert_prp*)((self)->prp) )
#define ctrl_raibert_type(self)    ( ctrl_raibert_get_prp(self)->type )
#define ctrl_raibert_end_of_thrust(self)   ( ctrl_raibert_get_prp(self)->end_of_thrust )
#define ctrl_raibert_end_of_thrust_t(self) ( ctrl_raibert_end_of_thrust(self).t )
#define ctrl_raibert_end_of_thrust_z(self) ( ctrl_raibert_end_of_thrust(self).z )
#define ctrl_raibert_end_of_thrust_v(self) ( ctrl_raibert_end_of_thrust(self).v )

#define ctrl_raibert_set_delta(self,val) ( ctrl_raibert_delta(self) = (val) )
#define ctrl_raibert_set_tau(self,val)   ( ctrl_raibert_tau(self)   = (val) )
#define ctrl_raibert_set_gamma(self,val) ( ctrl_raibert_gamma(self) = (val) )
#define ctrl_raibert_set_yeta1(self,val) ( ctrl_raibert_yeta1(self) = (val) )
#define ctrl_raibert_set_zr(self,val)    ( ctrl_raibert_zr(self)    = (val) )
#define ctrl_raibert_set_mu(self,val)    ( ctrl_raibert_mu(self)    = (val) )

ctrl_t *ctrl_raibert_set_params(ctrl_t *self, double delta, double tau, double gamma, double yeta1, double zr, double mu);
#define ctrl_raibert_set_params_full_nonlinear(self, delta, tau, gamma, yeta1, mu) \
  ctrl_raibert_set_params(self, delta, tau, gamma, yeta1, 0.0, mu)
#define ctrl_raibert_set_params_simplified_nonlinear(self, tau, yeta1) \
  ctrl_raibert_set_params(self, 0.0, tau, 0.0, yeta1, 0.0, 0.0)
#define ctrl_raibert_set_params_full_linear(self, delta, tau, gamma, yeta1, zr, mu) \
  ctrl_raibert_set_params(self, delta, tau, gamma, yeta1, zr, mu)
#define ctrl_raibert_set_params_simplified_linear(self, delta, tau, gamma, yeta1, zr) \
  ctrl_raibert_set_params(self, delta, tau, gamma, yeta1, zr, 0.0)

#define ctrl_raibert_create_full_nonlinear(self, cmd, model, delta, tau, gamma, yeta1, mu) \
  do { \
    ctrl_raibert_create(self, cmd, model, full_nonlinear); \
    ctrl_raibert_set_params_full_nonlinear(self, delta, tau, gamma, yeta1, mu); \
  } while (0)
#define ctrl_raibert_create_simplified_nonlinear(self, cmd, model, tau, yeta1) \
  do { \
    ctrl_raibert_create(self, cmd, model, simplified_nonlinear); \
    ctrl_raibert_set_params_simplified_nonlinear(self, tau, yeta1); \
  } while (0)
#define ctrl_raibert_create_full_linear(self, cmd, model, delta, tau, gamma, yeta1, zr, mu) \
  do { \
    ctrl_raibert_create(self, cmd, model, full_linear); \
    ctrl_raibert_set_params_full_linear(self, delta, tau, gamma, yeta1, zr, mu); \
  } while (0)
#define ctrl_raibert_create_simplified_linear(self, cmd, model, delta, tau, gamma, yeta1, zr) \
  do { \
    ctrl_raibert_create(self, cmd, model, simplified_linear); \
    ctrl_raibert_set_params_simplified_linear(self, delta, tau, gamma, yeta1, zr); \
  } while (0)

bool ctrl_raibert_is_in_thrust(ctrl_t *self);

double ctrl_raibert_calc_fz(ctrl_t *self, double t, vec_t p);

#endif /* __RHC_CTRL_RAIBERT_H__ */
