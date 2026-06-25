// RAII C++ wrappers around the C library objects that own heap
// resources, shared across binding translation units. Plain value
// structs (cmd_t, model_t, complex_t) are bound directly and do not need
// wrappers.
#pragma once

#include <cstddef>
#include <new>

#include "rhc_api.hpp"

namespace rhcpy {

// Owns a heap-allocated vec_t and frees it on destruction.
struct Vec {
  vec_t v = nullptr;

  explicit Vec(std::size_t n) {
    v = vec_create(n);
    if (v == nullptr) {
      throw std::bad_alloc();
    }
  }

  // Take ownership of an already-created vec_t (e.g. from vec_clone).
  explicit Vec(vec_t taken) : v(taken) {}

  ~Vec() {
    if (v != nullptr) {
      vec_destroy(v);
    }
  }

  Vec(const Vec &) = delete;
  Vec &operator=(const Vec &) = delete;

  [[nodiscard]] std::size_t size() const { return vec_size(v); }
};

// Base wrapper around a ctrl_t. The concrete controller is built in place
// by a *_create function; ctrl_destroy() dispatches to the right teardown
// via the controller's _destroy function pointer.
struct Ctrl {
  ctrl_t c{};
  bool created = false;

  Ctrl() = default;
  virtual ~Ctrl() {
    if (created) {
      ctrl_destroy(&c);
    }
  }

  Ctrl(const Ctrl &) = delete;
  Ctrl &operator=(const Ctrl &) = delete;
};

// The dynamics-morphing controller. Distinct C++ type so pybind11 can give
// it dynmorph-specific methods on top of the shared Ctrl accessors.
struct DynmorphCtrl : Ctrl {};

}  // namespace rhcpy
