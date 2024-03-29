#ifndef __RHC_CTRL_REGULATOR_H__
#define __RHC_CTRL_REGULATOR_H__

#include "rhc_ctrl.h"

typedef struct{
  double xi;
} ctrl_regulator_prp;

#define ctrl_regulator_q1(self) ( ctrl_cmd(self)->regulator.q1 )
#define ctrl_regulator_q2(self) ( ctrl_cmd(self)->regulator.q2 )
#define ctrl_regulator_xi(self) ( ((ctrl_regulator_prp*)((self)->prp))->xi )

cmd_t *ctrl_regulator_cmd_init(ctrl_t *self, cmd_t *cmd);
ctrl_t *ctrl_regulator_create(ctrl_t *self, cmd_t *cmd, model_t *model);
void ctrl_regulator_destroy(ctrl_t *self);
ctrl_t *ctrl_regulator_update(ctrl_t *self, double t, vec_t p);
void ctrl_regulator_header(FILE *fp, void *util);
void ctrl_regulator_writer(FILE *fp, ctrl_t *self, void *util);

double ctrl_regulator_calc_sqr_xi(double z0);
#define ctrl_regulator_calc_xi(z0) sqrt( ctrl_regulator_calc_sqr_xi( z0 ) )
double ctrl_regulator_calc_fz(ctrl_t *self, vec_t p);

#endif /* __RHC_CTRL_REGULATOR_H__ */
