#ifndef __RHC_LOGGER_H__
#define __RHC_LOGGER_H__

#include "rhc_defs.h"

/* definition of _simulator_t exists in rhc_simulator.h */
typedef struct _simulator_t simulator_t;

typedef void (*logger_header_fp_t)(FILE* fp, simulator_t *simulator, void *util);
typedef void (*logger_writer_fp_t)(FILE* fp, simulator_t *simulator, void *util);

#define EOL_BUFSIZ 8

typedef struct{
  char filename[RHC_BUFSIZ];
  FILE* fp;
  logger_header_fp_t header;
  logger_writer_fp_t writer;
  bool header_written_flag;
  char eol[EOL_BUFSIZ];
} logger_t;

#define logger_filename(self)          (self)->filename
#define logger_is_open(self)           ( (self)->fp != NULL )
#define logger_is_header_written(self) (self)->header_written_flag
#define logger_eol(self)               (self)->eol

logger_t *logger_init(logger_t *self);
void logger_destroy(logger_t *self);
logger_t *logger_create(const char *filename, logger_header_fp_t header, logger_writer_fp_t writer);
logger_t *logger_set_eol(logger_t *self, const char *eol);

FILE* logger_open(logger_t *self, const char *filename);
void logger_close(logger_t *self);
void logger_register(logger_t *self, logger_header_fp_t header, logger_writer_fp_t writer);
void logger_delegate(logger_t *src, logger_t *dst);
logger_t *logger_reset(logger_t *self, const char *filename);

void logger_write_header(logger_t *self, simulator_t *simulator, void *util);
void logger_write_data(logger_t *self, simulator_t *simulator, void *util);
void logger_write(logger_t *self, simulator_t *simulator, void *util);

#endif /* __RHC_LOGGER_H__ */
