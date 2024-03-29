#include "rhc_logger.h"
#include "rhc_string.h"

logger_t *logger_init(logger_t *self)
{
  self->filename[0] = '\0';
  self->fp = NULL;
  self->header = NULL;
  self->writer = NULL;
  self->header_written_flag = false;
  return self;
}

void logger_destroy(logger_t *self)
{
  logger_close( self );
}

logger_t *logger_create(const char *filename, logger_header_fp_t header, logger_writer_fp_t writer)
{
  logger_t *logger;

  if( ( logger = nalloc( logger_t, 1 ) ) == NULL ) {
    ALLOC_ERR();
    return NULL;
  }
  logger_open( logger, filename );
  logger_register( logger, header, writer );
  return logger;
}

FILE* logger_open(logger_t *self, const char *filename)
{
  logger_init( self );
  string_copy(filename, self->filename);
  self->fp = fopen(filename, "w");
  if(!self->fp) {
    RUNTIME_ERR(filename);
    return NULL;
  }
  return self->fp;
}

void logger_close(logger_t *self)
{
  if( self->fp )
    fclose( self->fp );
  self->fp = NULL;
}

void logger_register(logger_t *self, logger_header_fp_t header, logger_writer_fp_t writer)
{
  self->header = header;
  self->writer = writer;
}

void logger_delegate(logger_t *src, logger_t *dst)
{
  logger_init( dst );
  string_copy( logger_filename(src), logger_filename(dst) );
  dst->fp = src->fp;
  dst->header = src->header;
  dst->writer = src->writer;
  logger_init( src );
}

void logger_write_header(logger_t *self, simulator_t *simulator, void *util)
{
  if( !self->header )
    return;

  if( logger_is_open(self) ){
    self->header( self->fp, simulator, util );
  } else {
    self->header( stdout, simulator, util );
  }
  self->header_written_flag = true;
}

void logger_write_data(logger_t *self, simulator_t *simulator, void *util)
{
  if( !self->writer ) {
    RUNTIME_WARN( "No logger output" );
    return;
  }
  if( logger_is_open(self) ) {
    self->writer( self->fp, simulator, util );
  } else {
    self->writer( stdout, simulator, util );
  }
}

void logger_write(logger_t *self, simulator_t *simulator, void *util)
{
  if( !logger_is_header_written(self) )
    logger_write_header( self, simulator, util );
  logger_write_data( self, simulator, util );
}
