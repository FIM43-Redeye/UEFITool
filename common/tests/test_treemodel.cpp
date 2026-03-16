#include <catch_amalgamated.hpp>

#include "treemodel.h"
#include "ffsops.h"
#include "ffs.h"

TEST_CASE("Smoke test: treemodel compiles and links", "[treemodel][smoke]") {
    TreeModel model;
    REQUIRE(model.columnCount() == 5);
}
