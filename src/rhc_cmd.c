#include "rhc_cmd.h"

static const cmd_t __empty_cmd;

cmd_t *cmd_init(cmd_t *self)
{
  /* This function initializes the struct that the pointer self points
   * to with zeros. This supposes that the object __empty_cmd is
   * zero-initialized as static objects with no initializers are
   * implicitly initialized with zeros. */
  *self = __empty_cmd;
  return self;
}

cmd_t *cmd_default_init(cmd_t *self)
{
  cmd_init( self );
  self->za = 0.28;
  self->zh = 0.26;
  self->zm = 0.255;
  self->zb = 0.23;
  return self;
}

void cmd_destroy(cmd_t *self)
{
  cmd_init( self );
}

cmd_t *cmd_set(cmd_t *self, double za, double zh, double zm, double zb)
{
  self->za = za;
  self->zh = zh;
  self->zm = zm;
  self->zb = zb;
  return self;
}

cmd_t *cmd_copy(cmd_t *src, cmd_t *dst)
{
  memcpy(dst, src, sizeof(cmd_t));
  return dst;
}

void cmd_f_write(FILE* fp, cmd_t *self)
{
  fprintf( fp, "za = %f\n", self->za );
  fprintf( fp, "zh = %f\n", self->zh );
  fprintf( fp, "zm = %f\n", self->zm );
  fprintf( fp, "zb = %f\n", self->zb );
}

void cmd_f_write_regulator(FILE* fp, cmd_t *self)
{
  cmd_f_write( fp, self );
  fprintf( fp, "q1 = %f\n", self->regulator.q1 );
  fprintf( fp, "q2 = %f\n", self->regulator.q2 );
}

void cmd_f_write_dynmorph(FILE* fp, cmd_t *self)
{
  cmd_f_write( fp, self );
  fprintf( fp, "rho = %f\n", self->dynmorph.rho );
  fprintf( fp, "k = %f\n", self->dynmorph.k );
  fprintf( fp, "q_scale = %f\n", self->dynmorph.q_scale );
  fprintf( fp, "soft_landing = %s\n", self->dynmorph.soft_landing ? "true" : "false" );
}

void cmd_f_write_raibert(FILE* fp, cmd_t *self)
{
  cmd_f_write( fp, self );
  fprintf( fp, "delta = %f\n", self->raibert.delta );
  fprintf( fp, "tau = %f\n", self->raibert.tau );
  fprintf( fp, "gamma = %f\n", self->raibert.gamma );
  fprintf( fp, "yeta1 = %f\n", self->raibert.yeta1 );
  fprintf( fp, "zr = %f\n", self->raibert.zr );
  fprintf( fp, "mu = %f\n", self->raibert.mu );
}

void cmd_f_write_arl(FILE* fp, cmd_t *self)
{
  cmd_f_write( fp, self );
  fprintf( fp, "k = %f\n", self->arl.k );
  fprintf( fp, "beta = %f\n", self->arl.beta );
}

void cmd_f_write_mtoka(FILE* fp, cmd_t *self)
{
  cmd_f_write( fp, self );
  fprintf( fp, "tau = %f\n", self->mtoka.tau );
  fprintf( fp, "T = %f\n", self->mtoka.T );
  fprintf( fp, "a = %f\n", self->mtoka.a );
  fprintf( fp, "b = %f\n", self->mtoka.b );
  fprintf( fp, "c = %f\n", self->mtoka.c );
  fprintf( fp, "th = %f\n", self->mtoka.th );
  fprintf( fp, "mu = %f\n", self->mtoka.mu );
  fprintf( fp, "rho = %f\n", self->mtoka.rho );
  fprintf( fp, "lam = %f\n", self->mtoka.lam );
}
