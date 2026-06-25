#include <pybind11/numpy.h>

#include <cstring>
#include <new>
#include <stdexcept>
#include <string>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_vec(py::module_ &m) {
  py::class_<Vec>(m, "Vec", "A dense vector backed by the C library's vec_t.")
      .def(py::init([](std::size_t n) { return new Vec(n); }), py::arg("size"),
           "Create a zero-initialised vector of the given size.")
      .def(py::init([](const py::sequence &seq) {
             const std::size_t n = py::len(seq);
             auto *r = new Vec(n);
             for (std::size_t i = 0; i < n; ++i) {
               vec_set_elem(r->v, i, py::cast<double>(seq[i]));
             }
             return r;
           }),
           py::arg("values"), "Create a vector from a sequence of floats.")
      .def("__len__", [](const Vec &self) { return self.size(); })
      .def("__getitem__",
           [](const Vec &self, std::size_t i) {
             if (i >= self.size()) {
               throw py::index_error();
             }
             return vec_elem(self.v, i);
           })
      .def("__setitem__",
           [](Vec &self, std::size_t i, double val) {
             if (i >= self.size()) {
               throw py::index_error();
             }
             vec_set_elem(self.v, i, val);
           })
      .def_property_readonly("size", [](const Vec &self) { return self.size(); })
      .def(
          "to_numpy",
          [](const Vec &self) {
            auto result = py::array_t<double>(static_cast<py::ssize_t>(self.size()));
            std::memcpy(result.mutable_data(), vec_buf(self.v), self.size() * sizeof(double));
            return result;
          },
          "Return a NumPy copy of the vector contents.")
      .def("clone",
           [](const Vec &self) {
             vec_t c = vec_clone(self.v);
             if (c == nullptr) {
               throw std::bad_alloc();
             }
             return new Vec(c);
           })
      .def(
          "dot",
          [](const Vec &a, const Vec &b) {
            if (a.size() != b.size()) {
              throw std::invalid_argument("size mismatch");
            }
            return vec_dot(a.v, b.v);
          },
          py::arg("other"))
      .def("norm", [](const Vec &self) { return vec_norm(self.v); })
      .def(
          "__add__",
          [](const Vec &a, const Vec &b) {
            if (a.size() != b.size()) {
              throw std::invalid_argument("size mismatch");
            }
            auto *r = new Vec(a.size());
            vec_add(a.v, b.v, r->v);
            return r;
          },
          py::is_operator())
      .def(
          "__sub__",
          [](const Vec &a, const Vec &b) {
            if (a.size() != b.size()) {
              throw std::invalid_argument("size mismatch");
            }
            auto *r = new Vec(a.size());
            vec_sub(a.v, b.v, r->v);
            return r;
          },
          py::is_operator())
      .def(
          "__mul__",
          [](const Vec &a, double k) {
            auto *r = new Vec(a.size());
            vec_mul(a.v, k, r->v);
            return r;
          },
          py::is_operator())
      .def(
          "__eq__",
          [](const Vec &a, const Vec &b) {
            if (a.size() != b.size()) {
              return false;
            }
            return static_cast<bool>(vec_equal(a.v, b.v));
          },
          py::is_operator())
      .def("__repr__", [](const Vec &self) {
        std::string s = "Vec([";
        for (std::size_t i = 0; i < self.size(); ++i) {
          if (i != 0) {
            s += ", ";
          }
          s += std::to_string(vec_elem(self.v, i));
        }
        s += "])";
        return s;
      });
}

}  // namespace rhcpy
