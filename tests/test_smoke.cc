#include "test_harness.h"

TEST(smoke_test_runs) {
    ASSERT_EQ(1 + 1, 2);
    return true;
}
