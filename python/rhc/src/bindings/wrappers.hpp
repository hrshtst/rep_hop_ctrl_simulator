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

}  // namespace rhcpy
