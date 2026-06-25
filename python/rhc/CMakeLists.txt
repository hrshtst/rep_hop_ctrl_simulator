cmake_minimum_required(VERSION 3.15)
project(rhc LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 CONFIG REQUIRED)

# The C library lives at the repository root, two levels up from here.
set(RHC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
file(GLOB RHC_SRC CONFIGURE_DEPENDS "${RHC_ROOT}/src/*.c")

file(GLOB BINDINGS_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/bindings/*.cpp")

pybind11_add_module(_rhc ${BINDINGS_SRC} ${RHC_SRC})
target_include_directories(_rhc PRIVATE "${RHC_ROOT}/include")
target_link_libraries(_rhc PRIVATE m)
# The C library is numeric and benefits from optimization.
target_compile_options(_rhc PRIVATE $<$<COMPILE_LANGUAGE:C>:-O3>)

install(TARGETS _rhc DESTINATION rhc)
