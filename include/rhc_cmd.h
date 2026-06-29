#ifndef __RHC_CMD_H__
#define __RHC_CMD_H__

#include "rhc_misc.h"

typedef struct{
  double za;       /* apex height */
  double zh;       /* lift-off height */
  double zm;       /* standing height */
  double zb;       /* crouching height */
  union {
    struct _regulator {
      double q1, q2;
    } regulator;
    struct _dynmorph {
      double rho, k, q_scale;
      bool soft_landing;
    } dynmorph;
    struct _raibert {
      double delta, tau, gamma, yeta1, zr, mu;
    } raibert;
    struct _arl {
      double k, beta;
    } arl;
    struct _mtoka {
      double tau, T, a, b, c, th, mu, rho, lam;
    } mtoka;
  };
} cmd_t;

cmd_t *cmd_init(cmd_t *self);
cmd_t *cmd_default_init(cmd_t *self);
void cmd_destroy(cmd_t *self);

cmd_t *cmd_set(cmd_t *self, double za, double zh, double zm, double zb);

cmd_t *cmd_copy(cmd_t *src, cmd_t *dst);

void cmd_f_write(FILE *fp, cmd_t *self);
void cmd_f_write_regulator(FILE *fp, cmd_t *self);
void cmd_f_write_dynmorph(FILE *fp, cmd_t *self);
void cmd_f_write_raibert(FILE *fp, cmd_t *self);
void cmd_f_write_arl(FILE *fp, cmd_t *self);
void cmd_f_write_mtoka(FILE *fp, cmd_t *self);
#define cmd_write(self)               cmd_f_write( stdout, self )
#define cmd_write_regulator(self)     cmd_f_write_regulator( stdout, self )
#define cmd_write_dynmorph(self) cmd_f_write_dynmorph( stdout, self )
#define cmd_write_raibert(self)       cmd_f_write_raibert( stdout, self )
#define cmd_write_arl(self)           cmd_f_write_arl( stdout, self )
#define cmd_write_mtoka(self)         cmd_f_write_mtoka( stdout, self )

#endif /* __RHC_CMD_H__ */
