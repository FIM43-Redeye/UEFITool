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

TEST_CASE("guidDatabaseExportToFile", "[guiddatabase]") {
    SECTION("export and re-import round-trips") {
        // Load a known database
        auto path = writeTempCsv(
            "8C8CE578-8A3D-4F1C-9935-896185C32DD3,FFS2\n"
            "5473C07A-3DCB-4DCA-BD6F-1E9689E7349A,FFS3\n"
        );
        UINT32 numEntries = 0;
        initGuidDatabase(path, &numEntries);
        removeTempCsv();
        REQUIRE(numEntries == 2);

        // Build a GuidDatabase manually for export
        GuidDatabase db;
        EFI_GUID ffs2 = {0x8C8CE578, 0x8A3D, 0x4F1C,
                         {0x99, 0x35, 0x89, 0x61, 0x85, 0xC3, 0x2D, 0xD3}};
        EFI_GUID ffs3 = {0x5473C07A, 0x3DCB, 0x4DCA,
                         {0xBD, 0x6F, 0x1E, 0x96, 0x89, 0xE7, 0x34, 0x9A}};
        db[ffs2] = UString("FFS2");
        db[ffs3] = UString("FFS3");

        // Export to file
        UString exportPath("/tmp/uefitool_test_export.csv");
        REQUIRE(guidDatabaseExportToFile(exportPath, db) == U_SUCCESS);

        // Re-import
        initGuidDatabase(exportPath, &numEntries);
        REQUIRE(numEntries == 2);
        REQUIRE(guidDatabaseLookup(ffs2) == UString("FFS2"));
        REQUIRE(guidDatabaseLookup(ffs3) == UString("FFS3"));

        std::remove("/tmp/uefitool_test_export.csv");
    }
}

TEST_CASE("OperatorLessForGuids", "[guiddatabase]") {
    OperatorLessForGuids cmp;
    EFI_GUID a = {0x00000001, 0, 0, {0}};
    EFI_GUID b = {0x00000002, 0, 0, {0}};
    EFI_GUID a2 = {0x00000001, 0, 0, {0}};

    SECTION("consistent ordering") {
        REQUIRE(cmp(a, b) == true);
        REQUIRE(cmp(b, a) == false);
    }

    SECTION("equal GUIDs: not less") {
        REQUIRE(cmp(a, a2) == false);
        REQUIRE(cmp(a2, a) == false);
    }
}
