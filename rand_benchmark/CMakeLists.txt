# CMakeList.txt : CMake project for rand_benchmark, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (rand_benchmark "rand_benchmark.cpp" "rand_benchmark.h" "nanobench.h")

# TODO: Add tests and install targets if needed.
