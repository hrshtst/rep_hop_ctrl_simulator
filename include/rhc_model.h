#ifndef __RHC_MODEL_H__
#define __RHC_MODEL_H__

#include "rhc_vec.h"
#include "rhc_cmd.h"

#define G  9.806652

typedef struct{
  double m;
  cmd_t *cmd;
} model_t;

#define model_mass(self) ( (self)->m )
#define model_cmd(self)  ( (self)->cmd )

#define model_set_mass(self,m) ( model_mass(self) = (m) )
#define model_set_cmd(self,c)  ( model_cmd(self) = (c) )

model_t *model_init(model_t *self, double m, cmd_t *cmd);
void model_destroy(model_t *self);

double model_calc_acc(model_t *self, double z, double z0, double fz, double fe);

#endif /* __RHC_MODEL_H__ */
