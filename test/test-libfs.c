#include <stdlib.h>
#include "ctest.h"

// basic test without setup/teardown
CTEST(suite1, test1) {
}

// there are many different ASSERT macro's (see ctest.h)
CTEST(suite1, test2) {
    ASSERT_EQUAL(1,1);
}

CTEST(suite2, test1) {
    ASSERT_STR("foo", "foo");
}

CTEST(suite3, test3) {
    ASSERT_NOT_EQUAL(1,2);
}
