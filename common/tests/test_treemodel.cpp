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

// ---------------------------------------------------------------------------
// TreeModel tests (Tasks 4-6)
// ---------------------------------------------------------------------------

// Helper to add a simple item to the model. Reduces boilerplate.
static UModelIndex addTestItem(TreeModel& model, UINT8 type, UINT8 subtype,
                               const UString& name, UINT32 offset = 0,
                               const UByteArray& header = UByteArray(),
                               const UByteArray& body = UByteArray(),
                               const UByteArray& tail = UByteArray(),
                               const UModelIndex& parent = UModelIndex(),
                               UINT8 mode = CREATE_MODE_APPEND) {
    return model.addItem(offset, type, subtype, name, UString(), UString(),
                         header, body, tail, Movable, parent, mode);
}

TEST_CASE("TreeModel construction", "[treemodel]") {
    TreeModel model;

    SECTION("fresh model has no visible children") {
        REQUIRE(model.rowCount() == 0);
    }

    SECTION("columnCount is always 5") {
        REQUIRE(model.columnCount() == 5);
    }
}

TEST_CASE("TreeModel addItem and navigation", "[treemodel]") {
    TreeModel model;

    SECTION("append creates valid index") {
        auto idx = addTestItem(model, Types::Image, Subtypes::UefiImage, UString("img"));
        REQUIRE(idx.isValid());
        REQUIRE(model.rowCount() == 1);
        REQUIRE(model.name(idx) == UString("img"));
    }

    SECTION("multiple appends maintain order") {
        auto a = addTestItem(model, Types::Image, 0, UString("first"));
        auto b = addTestItem(model, Types::Image, 0, UString("second"));

        REQUIRE(model.rowCount() == 2);
        auto first = model.index(0, 0);
        auto second = model.index(1, 0);
        REQUIRE(model.name(first) == UString("first"));
        REQUIRE(model.name(second) == UString("second"));
    }

    SECTION("prepend inserts at beginning") {
        auto a = addTestItem(model, Types::Image, 0, UString("first"));
        auto b = addTestItem(model, Types::Image, 0, UString("prepended"),
                             0, UByteArray(), UByteArray(), UByteArray(),
                             UModelIndex(), CREATE_MODE_PREPEND);

        auto first = model.index(0, 0);
        REQUIRE(model.name(first) == UString("prepended"));
    }

    SECTION("insert before") {
        auto a = addTestItem(model, Types::Image, 0, UString("a"));
        auto b = addTestItem(model, Types::Image, 0, UString("b"));
        auto c = addTestItem(model, Types::Image, 0, UString("c"),
                             0, UByteArray(), UByteArray(), UByteArray(),
                             b, CREATE_MODE_BEFORE);

        REQUIRE(model.rowCount() == 3);
        REQUIRE(model.name(model.index(0, 0)) == UString("a"));
        REQUIRE(model.name(model.index(1, 0)) == UString("c"));
        REQUIRE(model.name(model.index(2, 0)) == UString("b"));
    }

    SECTION("insert after") {
        auto a = addTestItem(model, Types::Image, 0, UString("a"));
        auto b = addTestItem(model, Types::Image, 0, UString("b"));
        auto c = addTestItem(model, Types::Image, 0, UString("c"),
                             0, UByteArray(), UByteArray(), UByteArray(),
                             a, CREATE_MODE_AFTER);

        REQUIRE(model.rowCount() == 3);
        REQUIRE(model.name(model.index(0, 0)) == UString("a"));
        REQUIRE(model.name(model.index(1, 0)) == UString("c"));
        REQUIRE(model.name(model.index(2, 0)) == UString("b"));
    }

    SECTION("invalid mode returns invalid index") {
        auto idx = addTestItem(model, Types::Image, 0, UString("bad"),
                               0, UByteArray(), UByteArray(), UByteArray(),
                               UModelIndex(), 99);
        REQUIRE_FALSE(idx.isValid());
        REQUIRE(model.rowCount() == 0);
    }

    SECTION("parent of top-level item is invalid (root is hidden)") {
        auto img = addTestItem(model, Types::Image, 0, UString("img"));
        auto parent = model.parent(img);
        REQUIRE_FALSE(parent.isValid());
    }

    SECTION("grandchild navigation works") {
        auto img = addTestItem(model, Types::Image, 0, UString("img"));
        auto vol = addTestItem(model, Types::Volume, 0, UString("vol"),
                               0, UByteArray(), UByteArray(), UByteArray(), img);

        REQUIRE(model.rowCount(img) == 1);
        auto volIdx = model.index(0, 0, img);
        REQUIRE(model.name(volIdx) == UString("vol"));

        auto volParent = model.parent(volIdx);
        REQUIRE(volParent.isValid());
        REQUIRE(model.name(volParent) == UString("img"));
    }

    SECTION("rowCount with column > 0 returns 0") {
        addTestItem(model, Types::Image, 0, UString("img"));
        auto colIdx = model.index(0, 1);
        REQUIRE(model.rowCount(colIdx) == 0);
    }

    SECTION("index out of range returns invalid") {
        REQUIRE_FALSE(model.index(0, 0).isValid());  // No children yet
        addTestItem(model, Types::Image, 0, UString("img"));
        REQUIRE_FALSE(model.index(1, 0).isValid());   // Only 1 child
        REQUIRE_FALSE(model.index(0, 5).isValid());    // Column out of range
    }
}

TEST_CASE("TreeModel property round-trips", "[treemodel]") {
    TreeModel model;
    auto idx = addTestItem(model, Types::Image, 0, UString("img"));

    SECTION("name") {
        model.setName(idx, UString("renamed"));
        REQUIRE(model.name(idx) == UString("renamed"));
    }

    SECTION("text") {
        model.setText(idx, UString("some text"));
        REQUIRE(model.text(idx) == UString("some text"));
    }

    SECTION("info") {
        model.setInfo(idx, UString("some info"));
        REQUIRE(model.info(idx) == UString("some info"));
    }

    SECTION("addInfo append") {
        model.setInfo(idx, UString("base"));
        model.addInfo(idx, UString(" extra"), true);
        REQUIRE(model.info(idx) == UString("base extra"));
    }

    SECTION("addInfo prepend") {
        model.setInfo(idx, UString("base"));
        model.addInfo(idx, UString("prefix "), false);
        REQUIRE(model.info(idx) == UString("prefix base"));
    }

    SECTION("type and subtype") {
        model.setType(idx, Types::Volume);
        model.setSubtype(idx, Subtypes::Ffs3Volume);
        REQUIRE(model.type(idx) == Types::Volume);
        REQUIRE(model.subtype(idx) == Subtypes::Ffs3Volume);
    }

    SECTION("offset") {
        model.setOffset(idx, 0x1000);
        REQUIRE(model.offset(idx) == 0x1000);
    }

    SECTION("action") {
        REQUIRE(model.action(idx) == Actions::NoAction);
        model.setAction(idx, Actions::Remove);
        REQUIRE(model.action(idx) == Actions::Remove);
    }

    SECTION("marking") {
        model.setMarking(idx, BootGuardMarking::BootGuardFullyInRange);
        REQUIRE(model.marking(idx) == BootGuardMarking::BootGuardFullyInRange);
    }

    SECTION("compressed") {
        REQUIRE(model.compressed(idx) == false);
        model.setCompressed(idx, true);
        REQUIRE(model.compressed(idx) == true);
    }

    SECTION("all return defaults for invalid index") {
        UModelIndex invalid;
        REQUIRE(model.name(invalid) == UString());
        REQUIRE(model.text(invalid) == UString());
        REQUIRE(model.info(invalid) == UString());
        REQUIRE(model.header(invalid) == UByteArray());
        REQUIRE(model.body(invalid) == UByteArray());
        REQUIRE(model.tail(invalid) == UByteArray());
        REQUIRE(model.offset(invalid) == 0);
        REQUIRE(model.type(invalid) == 0);
        REQUIRE(model.subtype(invalid) == 0);
        REQUIRE(model.action(invalid) == Actions::NoAction);
        REQUIRE(model.fixed(invalid) == false);
        REQUIRE(model.compressed(invalid) == false);
        REQUIRE(model.hasEmptyHeader(invalid) == true);
        REQUIRE(model.hasEmptyBody(invalid) == true);
        REQUIRE(model.hasEmptyTail(invalid) == true);
    }
}

TEST_CASE("TreeModel::base calculation", "[treemodel]") {
    TreeModel model;

    SECTION("top-level item base equals its offset") {
        auto img = addTestItem(model, Types::Image, 0, UString("img"), 0x100);
        REQUIRE(model.base(img) == 0x100);
    }

    SECTION("grandchild base sums offsets") {
        auto img = addTestItem(model, Types::Image, 0, UString("img"), 0x100);
        auto vol = addTestItem(model, Types::Volume, 0, UString("vol"), 0x50,
                               UByteArray(), UByteArray(), UByteArray(), img);
        REQUIRE(model.base(vol) == 0x150);
    }

    SECTION("three levels deep") {
        auto img = addTestItem(model, Types::Image, 0, UString("img"), 0x1000);
        auto vol = addTestItem(model, Types::Volume, 0, UString("vol"), 0x200,
                               UByteArray(), UByteArray(), UByteArray(), img);
        auto file = addTestItem(model, Types::File, 0, UString("file"), 0x30,
                                UByteArray(), UByteArray(), UByteArray(), vol);
        REQUIRE(model.base(file) == 0x1230);
    }

    SECTION("invalid index returns 0") {
        REQUIRE(model.base(UModelIndex()) == 0);
    }
}
