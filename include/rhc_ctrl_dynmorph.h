#ifndef __RHC_CTRL_DYNMORPH_H__
#define __RHC_CTRL_DYNMORPH_H__

#include "rhc_ctrl.h"

enum ctrl_dynmorph_types{
  dynmorph_default = 0,
  soft_landing_piecewise,
  soft_landing_linear,
  fix_zb,
  fix_zm,
};

typedef struct{
  enum ctrl_dynmorph_types type;
  double (*_calc_phi_ratio)(double);
  ctrl_t* (*_update_params)(ctrl_t *, vec_t);
  cmd_t params;
  double q1, q2;
  double vm;
  bool cushioning;
} ctrl_dynmorph_prp;

#define ctrl_dynmorph_get_prp(self)    ( (ctrl_dynmorph_prp*)((self)->prp) )
#define ctrl_dynmorph_type(self)       ( ctrl_dynmorph_get_prp(self)->type )
#define ctrl_dynmorph_q1(self)         ( ctrl_dynmorph_get_prp(self)->q1 )
#define ctrl_dynmorph_q2(self)         ( ctrl_dynmorph_get_prp(self)->q2 )
#define ctrl_dynmorph_vm(self)         ( ctrl_dynmorph_get_prp(self)->vm )
#define ctrl_dynmorph_cushioning(self) ( ctrl_dynmorph_get_prp(self)->cushioning )
#define ctrl_dynmorph_r(self)          ( ctrl_dynmorph_get_prp(self)->r )
#define ctrl_dynmorph_sqr_gamma(self)  ( ctrl_dynmorph_get_prp(self)->sqr_gamma )
#define ctrl_dynmorph_f_gamma(self)    ( ctrl_dynmorph_get_prp(self)->f_gamma )
#define ctrl_dynmorph_phi(self)        ( ctrl_dynmorph_get_prp(self)->phi )

#define ctrl_dynmorph_rho(self)         ( ctrl_cmd(self)->dynmorph.rho )
#define ctrl_dynmorph_k(self)           ( ctrl_cmd(self)->dynmorph.k )
#define ctrl_dynmorph_q_scale(self)     ( ctrl_cmd(self)->dynmorph.q_scale )
#define ctrl_dynmorph_soft_landing(self) ( ctrl_cmd(self)->dynmorph.soft_landing )
#define ctrl_dynmorph_set_rho(self,val) ( ctrl_dynmorph_rho(self) = (val) )
#define ctrl_dynmorph_set_k(self,val)   ( ctrl_dynmorph_k(self) = (val) )
#define ctrl_dynmorph_set_q_scale(self,val) ( ctrl_dynmorph_q_scale(self) = (val) )
#define ctrl_dynmorph_enable_soft_landing(self) ( ctrl_dynmorph_soft_landing(self) = true )
#define ctrl_dynmorph_disable_soft_landing(self) ( ctrl_dynmorph_soft_landing(self) = false )

#define ctrl_dynmorph_params(self)     ( &(ctrl_dynmorph_get_prp(self)->params) )
#define ctrl_dynmorph_params_za(self)  ( ctrl_dynmorph_params(self)->za )
#define ctrl_dynmorph_params_zh(self)  ( ctrl_dynmorph_params(self)->zh )
#define ctrl_dynmorph_params_zm(self)  ( ctrl_dynmorph_params(self)->zm )
#define ctrl_dynmorph_params_zb(self)  ( ctrl_dynmorph_params(self)->zb )
#define ctrl_dynmorph_params_rho(self) ( ctrl_dynmorph_params(self)->dynmorph.rho )

double ctrl_dynmorph_calc_q1(double zh, double zm, double g);
double ctrl_dynmorph_calc_r(double zm, double zb);
double ctrl_dynmorph_calc_sqr_vm(double zh, double zm, double zb, double g);
#define ctrl_dynmorph_calc_vm(zh,zm,zb,g) sqrt( ctrl_dynmorph_calc_sqr_vm( zh, zm, zb, g ) )
double ctrl_dynmorph_calc_sqr_gamma(vec_t p, double zh, double zm, double zb, double g);
#define ctrl_dynmorph_calc_gamma(p,zh,zm,zb,g) sqrt( ctrl_dynmorph_calc_sqr_gamma( p, zh, zm, zb, g ) )
double ctrl_dynmorph_calc_gamma_lc(double rho, double k);
double ctrl_dynmorph_calc_za(double zh, double zm, double zb);
double ctrl_dynmorph_calc_zh(double za, double zm, double zb);
double ctrl_dynmorph_calc_zm(double za, double zh, double zb);
double ctrl_dynmorph_calc_zb(double za, double zh, double zm);
ctrl_t *ctrl_dynmorph_update_params_default(ctrl_t *self, vec_t p);
ctrl_t *ctrl_dynmorph_update_params_fix_zb(ctrl_t *self, vec_t p);
ctrl_t *ctrl_dynmorph_update_params_fix_zm(ctrl_t *self, vec_t p);

cmd_t *ctrl_dynmorph_cmd_init(ctrl_t *self, cmd_t *cmd);
ctrl_t *ctrl_dynmorph_create_with_type(ctrl_t *self, cmd_t *cmd, model_t *model, enum ctrl_dynmorph_types type);
#define ctrl_dynmorph_create(self, cmd, model) ctrl_dynmorph_create_with_type( self, cmd, model, dynmorph_default )
void ctrl_dynmorph_destroy(ctrl_t *self);
ctrl_t *ctrl_dynmorph_reset(ctrl_t *self, void *util);
ctrl_t *ctrl_dynmorph_update(ctrl_t *self, double t, vec_t p);
void ctrl_dynmorph_header(FILE *fp, void *util);
void ctrl_dynmorph_writer(FILE *fp, ctrl_t *self, void *util);
double ctrl_dynmorph_calc_fz(ctrl_t *self, vec_t p);


#endif /* __RHC_CTRL_DYNMORPH_H__ */
