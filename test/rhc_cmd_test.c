#include "rhc_cmd.h"
#include "rhc_test.h"

cmd_t cmd;

void setup()
{
  srand( 0 );
  cmd_init( &cmd );
}

void teardown()
{
  cmd_destroy( &cmd );
}

void set_rand(cmd_t *cmd)
{
  cmd->zd = rand();
  cmd->z0 = rand();
  cmd->zb = rand();
  cmd->raibert.delta = rand();
  cmd->raibert.tau = rand();
  cmd->raibert.gamma = rand();
  cmd->raibert.yeta1 = rand();
  cmd->raibert.zr = rand();
  cmd->raibert.mu = rand();
}

TEST(test_cmd_init)
{
  set_rand( &cmd );
  cmd_init( &cmd );
  ASSERT_EQ( 0, cmd.zd );
  ASSERT_EQ( 0, cmd.z0 );
  ASSERT_EQ( 0, cmd.zb );
}

TEST(test_cmd_init_regulator)
{
  set_rand( &cmd );
  cmd_init( &cmd );
  ASSERT_EQ( 0, cmd.regulator.q1 );
  ASSERT_EQ( 0, cmd.regulator.q2 );
}

TEST(test_cmd_init_raibert)
{
  set_rand( &cmd );
  cmd_init( &cmd );
  ASSERT_EQ( 0, cmd.raibert.delta );
  ASSERT_EQ( 0, cmd.raibert.tau );
  ASSERT_EQ( 0, cmd.raibert.gamma );
  ASSERT_EQ( 0, cmd.raibert.yeta1 );
  ASSERT_EQ( 0, cmd.raibert.zr );
  ASSERT_EQ( 0, cmd.raibert.mu );
}

TEST(test_cmd_default_init)
{
  set_rand( &cmd );
  cmd_default_init( &cmd );
  ASSERT_EQ( 0.28, cmd.zd );
  ASSERT_EQ( 0.26, cmd.z0 );
  ASSERT_EQ( 0.24, cmd.zb );
}


TEST(test_cmd_destroy)
{
  set_rand( &cmd );
  cmd_destroy( &cmd );
  ASSERT_EQ( 0, cmd.zd );
  ASSERT_EQ( 0, cmd.z0 );
  ASSERT_EQ( 0, cmd.zb );
}

TEST_SUITE(test_cmd)
{
  CONFIGURE_SUITE( setup, teardown );
  RUN_TEST(test_cmd_init);
  RUN_TEST(test_cmd_init_regulator);
  RUN_TEST(test_cmd_init_raibert);
  RUN_TEST(test_cmd_default_init);
  RUN_TEST(test_cmd_destroy);
}

int main(int argc, char *argv[])
{
  RUN_SUITE(test_cmd);
  TEST_REPORT();
  TEST_EXIT();
}
