#pragma once

// ============================================================================
// Board Selection — compile with -DBOARD_xxx to select target hardware
// ============================================================================

#if defined(BOARD_XIAO_S3)
#include "boards/xiao_esp32s3.h"
#elif defined(BOARD_TDONGLE_S3)
#include "boards/tdongle_s3.h"
#elif defined(BOARD_TDISPLAY_S3)
#include "boards/tdisplay_s3.h"
#elif defined(BOARD_TDISPLAY_S3_AMOLED)
#include "boards/tdisplay_s3_amoled.h"
#elif defined(BOARD_TBEAM_S3)
#include "boards/tbeam_s3.h"
#elif defined(BOARD_M5STICK_PLUS2)
#include "boards/m5stick_plus2.h"
#else
#error "No board defined! Add -DBOARD_xxx to build_flags"
#endif
