#include "rhc_string.h"
#include "rhc_test.h"

void setup()
{
}

void teardown()
{
}

TEST(test_string_copy)
{
  const char s[] = "test string";
  char d[BUFSIZ];
  char *ret;

  ret = string_copy(s, d);
  ASSERT_STREQ("test string", d);
  ASSERT_PTREQ(d, ret);
}

TEST(test_string_len)
{
  const char s[] = "test string";

  ASSERT_EQ( 11, string_len( s ) );
}

TEST(test_string_len_zero)
{
  const char s[] = "";

  ASSERT_EQ( 0, string_len( s ) );
}

TEST(test_string_is_digit)
{
  const char s1[] = "0123";
  const char s2[] = "01s01";

  ASSERT_TRUE( string_is_digit( s1 ) );
  ASSERT_FALSE( string_is_digit( s2 ) );
}

TEST_SUITE(test_string)
{
  CONFIGURE_SUITE( setup, teardown );
  RUN_TEST(test_string_copy);
  RUN_TEST(test_string_len);
  RUN_TEST(test_string_len_zero);
  RUN_TEST(test_string_is_digit);
}

int main(int arg, char *argv[])
{
  RUN_SUITE(test_string);
  TEST_REPORT();
  TEST_EXIT();
}
