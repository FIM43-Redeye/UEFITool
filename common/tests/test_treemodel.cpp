#include <catch_amalgamated.hpp>

#include "treemodel.h"
#include "ffsops.h"
#include "ffs.h"

TEST_CASE("Smoke test: treemodel compiles and links", "[treemodel][smoke]") {
    TreeModel model;
    REQUIRE(model.columnCount() == 5);
}

TEST_CASE("TreeItem construction", "[treeitem]") {
    UByteArray header("\x01\x02", 2);
    UByteArray body("\x03\x04\x05", 3);
    UByteArray tail("\x06", 1);

    TreeItem item(0x100, Types::File, 0, UString("TestFile"),
                  UString("test text"), UString("test info"),
                  header, body, tail, true, false);

    SECTION("all constructor values stored correctly") {
        REQUIRE(item.offset() == 0x100);
        REQUIRE(item.type() == Types::File);
        REQUIRE(item.subtype() == 0);
        REQUIRE(item.name() == UString("TestFile"));
        REQUIRE(item.text() == UString("test text"));
        REQUIRE(item.info() == UString("test info"));
        REQUIRE(item.header() == header);
        REQUIRE(item.body() == body);
        REQUIRE(item.tail() == tail);
        REQUIRE(item.fixed() == true);
        REQUIRE(item.compressed() == false);
    }

    SECTION("action defaults to NoAction") {
        REQUIRE(item.action() == Actions::NoAction);
    }

    SECTION("marking defaults to 0") {
        REQUIRE(item.marking() == 0);
    }

    SECTION("null parent is valid") {
        REQUIRE(item.parent() == nullptr);
    }

    SECTION("childCount starts at 0") {
        REQUIRE(item.childCount() == 0);
    }

    SECTION("columnCount is always 5") {
        REQUIRE(item.columnCount() == 5);
    }

    SECTION("row with no parent returns 0") {
        REQUIRE(item.row() == 0);
    }
}

TEST_CASE("TreeItem data concatenation", "[treeitem]") {
    SECTION("entire() returns header + body + tail") {
        UByteArray header("\x01\x02", 2);
        UByteArray body("\x03\x04\x05", 3);
        UByteArray tail("\x06", 1);
        TreeItem item(0, Types::File, 0, UString(), UString(), UString(),
                      header, body, tail, false, false);

        UByteArray expected("\x01\x02\x03\x04\x05\x06", 6);
        REQUIRE(item.entire() == expected);
    }

    SECTION("entire() with empty tail") {
        UByteArray header("\xAA", 1);
        UByteArray body("\xBB", 1);
        TreeItem item(0, Types::File, 0, UString(), UString(), UString(),
                      header, body, UByteArray(), false, false);

        UByteArray expected("\xAA\xBB", 2);
        REQUIRE(item.entire() == expected);
    }

    SECTION("entire() with all empty") {
        TreeItem item(0, Types::File, 0, UString(), UString(), UString(),
                      UByteArray(), UByteArray(), UByteArray(), false, false);
        REQUIRE(item.entire().isEmpty());
    }

    SECTION("addInfo appends") {
        TreeItem item(0, Types::File, 0, UString(), UString(), UString("base"),
                      UByteArray(), UByteArray(), UByteArray(), false, false);
        item.addInfo(UString(" added"), true);
        REQUIRE(item.info() == UString("base added"));
    }

    SECTION("addInfo prepends") {
        TreeItem item(0, Types::File, 0, UString(), UString(), UString("base"),
                      UByteArray(), UByteArray(), UByteArray(), false, false);
        item.addInfo(UString("prefix "), false);
        REQUIRE(item.info() == UString("prefix base"));
    }
}

TEST_CASE("TreeItem::data columns", "[treeitem]") {
    TreeItem item(0, Types::Volume, Subtypes::Ffs2Volume, UString("MyVol"),
                  UString("vol text"), UString("vol info"),
                  UByteArray(), UByteArray(), UByteArray(), false, false);

    REQUIRE(item.data(0) == UString("MyVol"));          // Name
    REQUIRE(item.data(1) == UString());                  // Action (NoAction = empty)
    REQUIRE(item.data(2) == UString("Volume"));          // Type
    REQUIRE(item.data(3) == UString("FFSv2"));           // Subtype
    REQUIRE(item.data(4) == UString("vol text"));        // Text
    REQUIRE(item.data(5) == UString());                  // Invalid column
    REQUIRE(item.data(-1) == UString());                 // Negative column
}

TEST_CASE("TreeItem child management", "[treeitem]") {
    // Parent owns children -- TreeItem destructor deletes them.
    auto makeChild = [](const UString& name, TreeItem* parent) {
        return new TreeItem(0, Types::File, 0, name, UString(), UString(),
                            UByteArray(), UByteArray(), UByteArray(),
                            false, false, parent);
    };

    SECTION("appendChild increases count and is retrievable") {
        TreeItem parent(0, Types::Volume, 0, UString("parent"), UString(), UString(),
                        UByteArray(), UByteArray(), UByteArray(), false, false);
        auto* child = makeChild(UString("child0"), &parent);
        parent.appendChild(child);

        REQUIRE(parent.childCount() == 1);
        REQUIRE(parent.child(0) == child);
        REQUIRE(child->row() == 0);
    }

    SECTION("prependChild inserts at front") {
        TreeItem parent(0, Types::Volume, 0, UString("parent"), UString(), UString(),
                        UByteArray(), UByteArray(), UByteArray(), false, false);
        auto* first = makeChild(UString("first"), &parent);
        auto* second = makeChild(UString("second"), &parent);
        parent.appendChild(first);
        parent.prependChild(second);

        REQUIRE(parent.childCount() == 2);
        REQUIRE(parent.child(0) == second);
        REQUIRE(parent.child(1) == first);
        REQUIRE(second->row() == 0);
        REQUIRE(first->row() == 1);
    }

    SECTION("insertChildBefore places correctly") {
        TreeItem parent(0, Types::Volume, 0, UString("parent"), UString(), UString(),
                        UByteArray(), UByteArray(), UByteArray(), false, false);
        auto* a = makeChild(UString("a"), &parent);
        auto* b = makeChild(UString("b"), &parent);
        auto* c = makeChild(UString("c"), &parent);
        parent.appendChild(a);
        parent.appendChild(b);

        REQUIRE(parent.insertChildBefore(b, c) == U_SUCCESS);
        REQUIRE(parent.childCount() == 3);
        REQUIRE(parent.child(0) == a);
        REQUIRE(parent.child(1) == c);
        REQUIRE(parent.child(2) == b);
    }

    SECTION("insertChildBefore with unknown item fails") {
        TreeItem parent(0, Types::Volume, 0, UString("parent"), UString(), UString(),
                        UByteArray(), UByteArray(), UByteArray(), false, false);
        TreeItem other(0, Types::File, 0, UString(), UString(), UString(),
                       UByteArray(), UByteArray(), UByteArray(), false, false);
        auto* child = makeChild(UString("child"), &parent);

        REQUIRE(parent.insertChildBefore(&other, child) == U_ITEM_NOT_FOUND);
        delete child;
    }

    SECTION("insertChildAfter places correctly") {
        TreeItem parent(0, Types::Volume, 0, UString("parent"), UString(), UString(),
                        UByteArray(), UByteArray(), UByteArray(), false, false);
        auto* a = makeChild(UString("a"), &parent);
        auto* b = makeChild(UString("b"), &parent);
        auto* c = makeChild(UString("c"), &parent);
        parent.appendChild(a);
        parent.appendChild(b);

        REQUIRE(parent.insertChildAfter(a, c) == U_SUCCESS);
        REQUIRE(parent.childCount() == 3);
        REQUIRE(parent.child(0) == a);
        REQUIRE(parent.child(1) == c);
        REQUIRE(parent.child(2) == b);
    }

    SECTION("insertChildAfter with unknown item fails") {
        TreeItem parent(0, Types::Volume, 0, UString("parent"), UString(), UString(),
                        UByteArray(), UByteArray(), UByteArray(), false, false);
        TreeItem other(0, Types::File, 0, UString(), UString(), UString(),
                       UByteArray(), UByteArray(), UByteArray(), false, false);
        auto* child = makeChild(UString("child"), &parent);

        REQUIRE(parent.insertChildAfter(&other, child) == U_ITEM_NOT_FOUND);
        delete child;
    }
}
