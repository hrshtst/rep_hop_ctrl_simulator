// Single point of inclusion for the C library headers. They are not
// wrapped in `extern "C"` themselves, so we do it here to give every
// declaration C linkage that matches the C definitions compiled from
// ../../src/*.c.
#pragma once

#include <pybind11/pybind11.h>

extern "C" {
#include "rhc_cmd.h"
#include "rhc_complex.h"
#include "rhc_ctrl.h"
#include "rhc_ctrl_dynmorph.h"
#include "rhc_ctrl_raibert.h"
#include "rhc_ctrl_regulator.h"
#include "rhc_model.h"
#include "rhc_simulator.h"
#include "rhc_vec.h"
}

// rhc_misc.h defines function-like macros `min`/`max` that collide with
// C++ <algorithm>; drop them so binding code can use the standard library.
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif
