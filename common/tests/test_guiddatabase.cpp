#include <catch_amalgamated.hpp>

#include "guiddatabase.h"

TEST_CASE("Smoke test: guiddatabase compiles and links", "[guiddatabase][smoke]") {
    UINT32 numEntries = 0;
    initGuidDatabase(UString(""), &numEntries);
    REQUIRE(numEntries == 0);
}
