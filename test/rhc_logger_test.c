#include "rhc_logger.h"
#include "rhc_simulator.h"
#include "rhc_test.h"

logger_t logger;
double t;
vec_t state;
cmd_t cmd;
model_t model;
ctrl_t ctrl;
simulator_t simulator;

const char TEST_LOG_FILENAME[] = "/tmp/rhc_test.log";

void assert_file(const char *filename, const char *expected_lines[], size_t n_lines)
{
  FILE *fp;

  if( ( fp = fopen( filename, "r" ) ) == NULL){
    FAIL( "Error opening file" );
  }

  char buffer[RHC_BUFSIZ]; // Buffer to hold each line from the file
  size_t i = 0;

  while( fgets( buffer, sizeof(buffer), fp ) ){
    // Remove trailing newline character from the file line
    size_t len = strlen( buffer );
    if( len > 0 && buffer[len - 1] == '\n' ){
      buffer[len - 1] = '\0';
    }

    // Check if we have more lines in the file than provided
    if( i >= n_lines ){
      fclose(fp);
      FAIL( "Number of lines in the file is greater than expected" );
    }

    // Compare the file line with the corresponding provided line
    if( strcmp(buffer, expected_lines[i] ) != 0 ){
      char msg[RHC_BUFSIZ];
      fclose(fp);
      snprintf( msg, RHC_BUFSIZ,
                "Line #%zu:\nExpected: %s\n But was: %s",
                i, expected_lines[i], buffer );
      FAIL( msg );
    }
    i++;
  }

  fclose(fp);
}

void setup()
{
  logger_init( &logger );
  t = 0;
  state = vec_create( 3 );
  vec_clear( state );
  cmd_init( &cmd );
  model_init( &model, 1.0 );
  ctrl_init( &ctrl, &cmd, &model );
  simulator_init( &simulator, &cmd, &ctrl, &model );
}

void teardown()
{
  logger_destroy( &logger );
  simulator_destroy( &simulator );
  ctrl_destroy( &ctrl );
  model_destroy( &model );
  cmd_destroy( &cmd );
  vec_destroy( state );
}

TEST(test_logger_init)
{
  logger_t l;
  logger_init( &l );
  ASSERT_EQ( '\0', logger_filename( &l )[0] );
  ASSERT_PTREQ( NULL, l.fp );
  ASSERT_PTREQ( NULL, l.header );
  ASSERT_PTREQ( NULL, l.writer );
  ASSERT_FALSE( logger_is_header_written( &l ) );
  ASSERT_STREQ( "\n", logger_eol( &l ) );
  logger_destroy( &l );
}

TEST(test_logger_destroy)
{
  logger_t l;
  logger_init( &l );
  logger_destroy( &l );
  ASSERT_PTREQ( NULL, l.fp );
}

TEST(test_logger_set_eol)
{
  ASSERT_STREQ( "\n", logger_eol( &logger ) );
  logger_set_eol( &logger, "\r\n" );
  ASSERT_STREQ( "\r\n", logger_eol( &logger ) );
}

TEST(test_logger_open)
{
  logger_open( &logger, TEST_LOG_FILENAME );
  ASSERT_STREQ( TEST_LOG_FILENAME, logger_filename(&logger) );
  ASSERT_PTRNE( NULL, logger.fp );
  ASSERT_PTRNE( stdout, logger.fp );
  ASSERT_PTREQ( NULL, logger.header );
  ASSERT_PTREQ( NULL, logger.writer );
  ASSERT_FALSE( logger_is_header_written( &logger ) );
  logger_close( &logger );
}

TEST(test_logger_open_null)
{
  logger_open( &logger, NULL );
  ASSERT_STREQ( "", logger_filename(&logger) );
  ASSERT_PTREQ( stdout, logger.fp );
  ASSERT_PTREQ( NULL, logger.header );
  ASSERT_PTREQ( NULL, logger.writer );
  ASSERT_FALSE( logger_is_header_written( &logger ) );
  logger_close( &logger );
}

TEST(test_logger_close)
{
  logger_open( &logger, TEST_LOG_FILENAME );
  logger_close( &logger );
  ASSERT_PTREQ( NULL, logger.fp );
}

void header(FILE *fp, simulator_t *s, void *util) {
  fprintf( fp, "t,z,vz,fe,za,zh,zb,m,az");
}

void output(FILE *fp, simulator_t *s, void *util) {
  vec_t state = simulator_state(s);
  cmd_t *cmd = simulator_cmd(s);
  model_t *model = simulator_model(s);
  fprintf( fp, "%f,%f,%f,%f,%f,%f,%f,%f,%f",
           simulator_time(s),
           vec_elem(state,0), vec_elem(state,1),
           simulator_fe(s), cmd->za, cmd->zh, cmd->zb,
           model_mass(model), model_acc(model) );
}

TEST(test_logger_register)
{
  logger_register( &logger, header, output );
  ASSERT_PTREQ( header, logger.header );
  ASSERT_PTREQ( output, logger.writer );
}

TEST(test_logger_delegate)
{
  logger_t src, dst;
  FILE *tmp_fp;
  bool is_header_written = false;
  char eol[3];

  logger_init( &src );
  logger_open( &src, TEST_LOG_FILENAME );
  tmp_fp = src.fp;
  logger_register( &src, header, output );
  is_header_written = logger_is_header_written( &src );
  string_copy( logger_eol(&src), eol );
  logger_delegate( &src, &dst );
  ASSERT_STREQ( "", logger_filename(&src) );
  ASSERT_PTREQ( NULL, src.fp );
  ASSERT_PTREQ( NULL, src.header );
  ASSERT_PTREQ( NULL, src.writer );
  ASSERT_STREQ( TEST_LOG_FILENAME, logger_filename(&dst) );
  ASSERT_PTREQ( tmp_fp, dst.fp );
  ASSERT_PTREQ( header, dst.header );
  ASSERT_PTREQ( output, dst.writer );
  ASSERT_STREQ( eol, logger_eol(&dst) );
  ASSERT_EQ( is_header_written, logger_is_header_written(&dst) );
  ASSERT_FALSE( logger_is_header_written(&dst) );
  logger_close( &src );
  logger_close( &dst );
}

TEST(test_logger_delegate_2)
{
  logger_t src, dst;
  FILE *tmp_fp;
  bool is_header_written = false;
  char eol[3];

  logger_init( &src );
  logger_open( &src, TEST_LOG_FILENAME );
  tmp_fp = src.fp;
  logger_register( &src, header, output );
  logger_write( &src, &simulator, NULL );
  is_header_written = logger_is_header_written( &src );
  string_copy( logger_eol(&src), eol );
  logger_delegate( &src, &dst );
  ASSERT_STREQ( "", logger_filename(&src) );
  ASSERT_PTREQ( NULL, src.fp );
  ASSERT_PTREQ( NULL, src.header );
  ASSERT_PTREQ( NULL, src.writer );
  ASSERT_STREQ( TEST_LOG_FILENAME, logger_filename(&dst) );
  ASSERT_PTREQ( tmp_fp, dst.fp );
  ASSERT_PTREQ( header, dst.header );
  ASSERT_PTREQ( output, dst.writer );
  ASSERT_STREQ( eol, logger_eol(&dst) );
  ASSERT_EQ( is_header_written, logger_is_header_written(&dst) );
  ASSERT_TRUE( logger_is_header_written(&dst) );
  logger_close( &src );
  logger_close( &dst );
}

TEST(test_logger_is_open)
{
  ASSERT_FALSE( logger_is_open(&logger) );
  logger_open( &logger, TEST_LOG_FILENAME );
  ASSERT_TRUE( logger_is_open(&logger) );
  logger_close( &logger );
  ASSERT_FALSE( logger_is_open(&logger) );
}

TEST(test_logger_create)
{
  logger_t *l = logger_create( TEST_LOG_FILENAME, header, output );
  ASSERT_STREQ( TEST_LOG_FILENAME, logger_filename(l) );
  ASSERT_PTRNE( NULL, l->fp );
  ASSERT_PTRNE( stdout, l->fp );
  ASSERT_PTREQ( header, l->header );
  ASSERT_PTREQ( output, l->writer );
  logger_destroy( l );
  free( l );
}

TEST(test_logger_reset)
{
  logger_open( &logger, NULL );
  logger_register( &logger, header, output );
  logger.header_written_flag = true;
  logger_reset( &logger, NULL );
  ASSERT_FALSE( logger_is_header_written( &logger ) );
  ASSERT_PTREQ( stdout, logger.fp );
  ASSERT_PTREQ( header, logger.header );
  ASSERT_PTREQ( output, logger.writer );
}

TEST(test_logger_reset_with_filename)
{
  const char filename1[] = "/tmp/rhc_logger_filename1.log";
  const char filename2[] = "/tmp/rhc_logger_filename2.log";

  logger_t *new_logger = logger_create( filename1, header, output );
  new_logger->header_written_flag = true;
  ASSERT_STREQ( filename1, logger_filename( new_logger ) );
  ASSERT_PTRNE( NULL, new_logger->fp );
  ASSERT_PTRNE( stdout, new_logger->fp );
  ASSERT_PTREQ( header, new_logger->header );
  ASSERT_PTREQ( output, new_logger->writer );
  ASSERT_TRUE( logger_is_header_written( new_logger ) );

  logger_reset( new_logger, filename2 );
  ASSERT_STREQ( filename2, logger_filename( new_logger ) );
  ASSERT_PTRNE( NULL, new_logger->fp );
  ASSERT_PTRNE( stdout, new_logger->fp );
  ASSERT_PTREQ( header, new_logger->header );
  ASSERT_PTREQ( output, new_logger->writer );
  ASSERT_FALSE( logger_is_header_written( new_logger ) );

  logger_destroy( new_logger );
  free( new_logger );
}

TEST(test_logger_reset_with_no_filename)
{
  const char filename1[] = "/tmp/rhc_logger_filename1.log";

  logger_t *new_logger = logger_create( filename1, header, output );
  new_logger->header_written_flag = true;
  ASSERT_STREQ( filename1, logger_filename( new_logger ) );
  ASSERT_PTRNE( NULL, new_logger->fp );
  ASSERT_PTRNE( stdout, new_logger->fp );
  ASSERT_PTREQ( header, new_logger->header );
  ASSERT_PTREQ( output, new_logger->writer );
  ASSERT_TRUE( logger_is_header_written( new_logger ) );

  logger_reset( new_logger, NULL );
  ASSERT_STREQ( "", logger_filename( new_logger ) );
  ASSERT_PTREQ( stdout, new_logger->fp );
  ASSERT_PTREQ( header, new_logger->header );
  ASSERT_PTREQ( output, new_logger->writer );
  ASSERT_FALSE( logger_is_header_written( new_logger ) );

  logger_destroy( new_logger );
  free( new_logger );
}

simulator_t *set_simulator_data(simulator_t *simulator, double t, double z, double vz, double fe)
{
  vec_t v;

  v = vec_create_list( 2, z, vz );
  simulator_set_time( simulator, t );
  simulator_set_state( simulator, v );
  simulator_set_fe( simulator, fe );
  vec_destroy( v );
  return simulator;
}

TEST(test_logger_write_header)
{
  logger_open( &logger, TEST_LOG_FILENAME );
  logger_register( &logger, header, output );
  ASSERT_FALSE( logger_is_header_written(&logger) );
  logger_write_header( &logger, &simulator, NULL );
  ASSERT_TRUE( logger_is_header_written(&logger) );
  logger_write_header( &logger, &simulator, NULL );
  ASSERT_TRUE( logger_is_header_written(&logger) );
  logger_close( &logger );
}

TEST(test_logger_write_data)
{
  const char *expected[] = {
    "0.000000,0.100000,0.200000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000",
    "0.001000,0.110000,0.220000,1.000000,0.000000,0.000000,0.000000,1.000000,0.000000",
  };

  logger_open( &logger, TEST_LOG_FILENAME );
  logger_register( &logger, NULL, output );
  set_simulator_data( &simulator, 0.000, 0.1, 0.2, 0.0 );
  logger_write( &logger, &simulator, NULL );
  set_simulator_data( &simulator, 0.001, 0.11, 0.22, 1.0 );
  logger_write( &logger, &simulator, NULL );
  logger_close( &logger );
  assert_file( TEST_LOG_FILENAME, expected, 3 );
}

TEST(test_logger_write)
{
  const char *expected[] = {
    "t,z,vz,fe,za,zh,zb,m,az",
    "0.000000,0.100000,0.200000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000",
    "0.003300,0.110000,0.220000,1.000000,0.000000,0.000000,0.000000,1.000000,0.000000",
  };

  logger_open( &logger, TEST_LOG_FILENAME );
  logger_register( &logger, header, output );
  set_simulator_data( &simulator, 0.000, 0.1, 0.2, 0.0 );
  logger_write( &logger, &simulator, NULL );
  set_simulator_data( &simulator, 0.0033, 0.11, 0.22, 1.0 );
  logger_write( &logger, &simulator, NULL );
  logger_close( &logger );
  assert_file( TEST_LOG_FILENAME, expected, 3 );
}

TEST(test_logger_write_not_regiseter_header)
{
  const char *expected[] = {
    "0.000000,0.100000,0.200000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000",
    "0.002000,0.110000,0.220000,1.000000,0.000000,0.000000,0.000000,1.000000,0.000000",
  };

  logger_open( &logger, TEST_LOG_FILENAME );
  logger_register( &logger, NULL, output );
  set_simulator_data( &simulator, 0.000, 0.1, 0.2, 0.0 );
  logger_write( &logger, &simulator, NULL );
  set_simulator_data( &simulator, 0.002, 0.11, 0.22, 1.0 );
  logger_write( &logger, &simulator, NULL );
  logger_close( &logger );
  assert_file( TEST_LOG_FILENAME, expected, 3 );
}

TEST_SUITE(test_logger)
{
  CONFIGURE_SUITE( setup, teardown );
  RUN_TEST( test_logger_init );
  RUN_TEST( test_logger_destroy );
  RUN_TEST( test_logger_set_eol );
  RUN_TEST( test_logger_open );
  RUN_TEST( test_logger_open_null );
  RUN_TEST( test_logger_close );
  RUN_TEST( test_logger_register );
  RUN_TEST( test_logger_delegate );
  RUN_TEST( test_logger_delegate_2 );
  RUN_TEST( test_logger_is_open );
  RUN_TEST( test_logger_create );
  RUN_TEST( test_logger_reset );
  RUN_TEST( test_logger_reset_with_filename );
  RUN_TEST( test_logger_reset_with_no_filename );
  RUN_TEST( test_logger_write_header );
  RUN_TEST( test_logger_write_data );
  RUN_TEST( test_logger_write );
  RUN_TEST( test_logger_write_not_regiseter_header );
}

int main(int argc, char *argv[])
{
  RUN_SUITE( test_logger );
  TEST_REPORT();
  TEST_EXIT();
}
