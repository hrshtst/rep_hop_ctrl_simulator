#ifndef __RHC_SIMULATOR_H__
#define __RHC_SIMULATOR_H__

#include "rhc_cmd.h"
#include "rhc_ctrl.h"
#include "rhc_logger.h"
#include "rhc_model.h"
#include "rhc_ode.h"
#include "rhc_string.h"

#define ERR_RESET_FAIL "user-given reset function faild"

typedef struct _simulator_t{
  double t;
  int step;
  ode_t ode;
  vec_t state;
  double fe;
  cmd_t *cmd;
  ctrl_t *ctrl;
  model_t *model;
  int n_trial;
  bool (*reset_fp)(struct _simulator_t*, void*);
  bool (*update_fp)(struct _simulator_t*, double, void*);
  void (*dump_fp)(struct _simulator_t*, logger_t*, void*);
  char tag[RHC_BUFSIZ];
} simulator_t;

#define simulator_cmd(self)   (self)->cmd
#define simulator_ctrl(self)  (self)->ctrl
#define simulator_model(self) (self)->model
#define simulator_time(self)  (self)->t
#define simulator_step(self)  (self)->step
#define simulator_state(self) (self)->state
#define simulator_fe(self)    (self)->fe
#define simulator_n_trial(self) (self)->n_trial
#define simulator_tag(self)   (self)->tag

#define simulator_set_state(self,p) vec_copy( p, simulator_state(self) )
#define simulator_set_time(self,t)  ( simulator_time(self)  = (t) )
#define simulator_inc_step(self)    ( simulator_step(self)++ )
#define simulator_set_fe(self,fe)   ( simulator_fe(self) = (fe) )
#define simulator_inc_trial(self)   ( simulator_n_trial(self)++ )

simulator_t *simulator_init(simulator_t *self, cmd_t *cmd, ctrl_t *ctrl, model_t *model);
void simulator_destroy(simulator_t *self);

void simulator_set_reset_fp(simulator_t *self, bool (*reset_fp)(simulator_t*, void*));
void simulator_set_update_fp(simulator_t *self, bool (*update_fp)(simulator_t*, double, void*));
void simulator_set_dump_fp(simulator_t *self, void (*dump_fp)(simulator_t*, logger_t*, void*));

char *simulator_set_tag(simulator_t *self, const char* tag);
char *simulator_update_default_tag(simulator_t *self);
bool simulator_has_default_tag(simulator_t *self);
vec_t simulator_dp(double t, vec_t x, void *util, vec_t v);
bool simulator_reset_ctrl(simulator_t *self, void *util);
bool simulator_reset(simulator_t *self, void *util);
bool simulator_update(simulator_t *self, double dt, void *util);
void simulator_update_time(simulator_t *self, double dt);
void simulator_run(simulator_t *self, vec_t p0, double time, double dt, logger_t *logger, void *util);

/* Runs a simulation like simulator_run but, instead of dumping to a
 * logger, records the per-step time series into caller-provided arrays
 * (each may be NULL to skip that column): t, z (=state[0]), vz
 * (=state[1]), fz (ground reaction force), and phase (the controller
 * phase enum as int). Samples are recorded at the same point as the
 * dump in simulator_run (before each integration step), up to
 * `capacity` samples or until `time` is reached, whichever comes first.
 * Returns the number of samples written. This keeps live consumers
 * (e.g. Python bindings) free of any file I/O. */
int simulator_rollout(simulator_t *self, vec_t p0, double time, double dt,
                      double *t, double *z, double *vz, double *fz, int *phase,
                      int capacity, void *util);

void simulator_header_default(FILE* fp, simulator_t *self, void *util);
void simulator_writer_default(FILE* fp, simulator_t *self, void *util);
void simulator_set_default_logger(simulator_t *self, logger_t *logger);

void simulator_dump(simulator_t *self, logger_t *logger, void *util);
void simulator_dump_header(simulator_t *self, logger_t *logger, void *util);
void simulator_dump_data(simulator_t *self, logger_t *logger, void *util);

#endif /* __RHC_SIMULATOR_H__ */
