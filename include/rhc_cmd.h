#ifndef __RHC_CMD_H__
#define __RHC_CMD_H__

#include "rhc_misc.h"

typedef struct{
  double zd;       /* desired position */
  double z0;       /* referential lift-off position */
  double zb;       /* referential crounching position */
  union {
    struct _regulator {
      double q1, q2;
    } regulator;
    struct _raibert {
      double delta, tau, gamma, yeta1, zr, mu;
    } raibert;
  };
} cmd_t;

cmd_t *cmd_init(cmd_t *self);
cmd_t *cmd_default_init(cmd_t *self);
void cmd_destroy(cmd_t *self);

#endif /* __RHC_CMD_H__ */
