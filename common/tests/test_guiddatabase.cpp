#include <cstdio>
#include <fstream>
#include <string>

#include <catch_amalgamated.hpp>

#include "guiddatabase.h"

// Helper: write a temporary CSV file and return its path.
static UString writeTempCsv(const std::string& content) {
    std::string path = "/tmp/uefitool_test_guids.csv";
    std::ofstream out(path, std::ios::trunc);
    out << content;
    out.close();
    return UString(path.c_str());
}

static void removeTempCsv() {
    std::remove("/tmp/uefitool_test_guids.csv");
}

TEST_CASE("initGuidDatabase", "[guiddatabase]") {
    SECTION("valid CSV loads entries") {
        auto path = writeTempCsv(
            "# Comment line\n"
            "8C8CE578-8A3D-4F1C-9935-896185C32DD3,FFS2\n"
            "5473C07A-3DCB-4DCA-BD6F-1E9689E7349A,FFS3\n"
        );
        UINT32 numEntries = 0;
        initGuidDatabase(path, &numEntries);
        REQUIRE(numEntries == 2);
        removeTempCsv();
    }

    SECTION("empty file loads 0 entries") {
        auto path = writeTempCsv("");
        UINT32 numEntries = 99;
        initGuidDatabase(path, &numEntries);
        REQUIRE(numEntries == 0);
        removeTempCsv();
    }

    SECTION("comments only loads 0 entries") {
        auto path = writeTempCsv("# just a comment\n# another\n");
        UINT32 numEntries = 99;
        initGuidDatabase(path, &numEntries);
        REQUIRE(numEntries == 0);
        removeTempCsv();
    }

    SECTION("malformed lines silently skipped") {
        auto path = writeTempCsv(
            "not-a-guid,BadEntry\n"
            "8C8CE578-8A3D-4F1C-9935-896185C32DD3,GoodEntry\n"
            "no-comma-at-all\n"
        );
        UINT32 numEntries = 0;
        initGuidDatabase(path, &numEntries);
        REQUIRE(numEntries == 1);
        removeTempCsv();
    }

    SECTION("re-init clears previous entries") {
        auto path = writeTempCsv(
            "8C8CE578-8A3D-4F1C-9935-896185C32DD3,First\n"
        );
        UINT32 numEntries = 0;
        initGuidDatabase(path, &numEntries);
        REQUIRE(numEntries == 1);

        auto path2 = writeTempCsv("");
        initGuidDatabase(path2, &numEntries);
        REQUIRE(numEntries == 0);
        removeTempCsv();
    }
}

TEST_CASE("guidDatabaseLookup", "[guiddatabase]") {
    auto path = writeTempCsv(
        "8C8CE578-8A3D-4F1C-9935-896185C32DD3,FFS2\n"
    );
    UINT32 numEntries = 0;
    initGuidDatabase(path, &numEntries);
    removeTempCsv();

    SECTION("known GUID returns name") {
        EFI_GUID ffs2 = {0x8C8CE578, 0x8A3D, 0x4F1C,
                         {0x99, 0x35, 0x89, 0x61, 0x85, 0xC3, 0x2D, 0xD3}};
        REQUIRE(guidDatabaseLookup(ffs2) == UString("FFS2"));
    }

    SECTION("unknown GUID returns empty string") {
        EFI_GUID unknown = {0xDEADBEEF, 0, 0, {0}};
        REQUIRE(guidDatabaseLookup(unknown) == UString());
    }
}
