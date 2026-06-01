cmake_minimum_required(VERSION 3.16)
project(aerie LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Top-level entry point — builds libfs and tests/benchmark together.
#
# Quick start:
#   cmake -S . -B build
#   cmake --build build --parallel $(nproc)
#
# Options (passed through to libfs):
#   -DRPC=net|fast|fast-two   RPC backend          (default: net)
#   -DSCMPOOL=user|kernel     SCM pool backend      (default: user)
#   -DBUILD_BENCH=ON|OFF      Build benchmarks      (default: ON)
#   -DCOVERAGE=ON             gcov instrumentation  (default: OFF)
# ---------------------------------------------------------------------------

add_subdirectory(libfs)
