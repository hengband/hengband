#include "load/load-util.h"
#include "save/item-writer.h"
#include "save/save-util.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "test/save/scoped-save-io.h"
#include "test/scoped-vector-wrapper.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>
#include <stdexcept>

TEST_CASE("Baseitem records serialize 32768 entries independently of definitions")
{
    test::ScopedVectorWrapper definitions_guard(BaseitemList::get_instance());
    test::ScopedVectorWrapper records_guard(BaseitemRecords::get_instance());
    BaseitemList::get_instance().resize(1);
    auto &records = BaseitemRecords::get_instance();
    records.initialize(32768);
    REQUIRE(records.size() == 32768);
    records.get_record(32767).mark_awareness(true);
    records.get_record(32767).mark_trial(true);
    CHECK(records.get_record(32767).is_aware());
    CHECK_THROWS_AS(records.get_record(-1), std::logic_error);

    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = test::preserve_save_io();
    saving_savefile = file;
    save_xor_byte = 0;
    v_stamp = x_stamp = 0;
    wr_baseitem_records();
    wr_u16b(0x1234);

    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    v_check = x_check = 0;
    CHECK(rd_u16b() == 32768);
    CHECK(rd_byte() == 0);
    strip_bytes(32766);
    CHECK(rd_byte() == 0x03);
    CHECK(rd_u16b() == 0x1234);
}
