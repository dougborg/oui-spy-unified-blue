#include <unity.h>

void test_native_arithmetic_sanity() {
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_native_arithmetic_sanity);
    return UNITY_END();
}
