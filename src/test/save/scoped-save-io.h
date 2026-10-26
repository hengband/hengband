#pragma once

#include "load/load-util.h"
#include "save/save-util.h"
#include "test/scoped-restore.h"

namespace test {

[[nodiscard]] inline auto preserve_save_io()
{
    return scoped_restore(saving_savefile, loading_savefile, save_xor_byte, load_xor_byte, v_stamp, x_stamp, v_check, x_check);
}

}
