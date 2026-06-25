// Declarations of the per-area binding registration functions. Each is
// defined in its own translation unit and called from module.cpp.
#pragma once

#include <pybind11/pybind11.h>

namespace rhcpy {

void register_vec(pybind11::module_ &m);
void register_complex(pybind11::module_ &m);
void register_cmd(pybind11::module_ &m);
void register_model(pybind11::module_ &m);
void register_ctrl(pybind11::module_ &m);
void register_dynmorph(pybind11::module_ &m);

}  // namespace rhcpy
