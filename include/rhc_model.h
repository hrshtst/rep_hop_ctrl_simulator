#ifndef __RHC_MODEL_H__
#define __RHC_MODEL_H__

#include "rhc_misc.h"

typedef struct{
  double m;
  double acc;
  double gravity;
} model_t;

#define model_mass(self)    (self)->m
#define model_acc(self)     (self)->acc
#define model_gravity(self) (self)->gravity

#define model_set_mass(self,m)    ( model_mass(self) = (m) )
#define model_set_acc(self,a)     ( model_acc(self) = (a) )
#define model_set_gravity(self,g) ( model_gravity(self) = (g) )

model_t *model_init(model_t *self, double m);
void model_destroy(model_t *self);

double model_calc_acc(double m, double fz, double fe, double g);

model_t *model_update(model_t *self, double fz, double fe);

#endif /* __RHC_MODEL_H__ */
