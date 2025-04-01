cmake_minimum_required(VERSION 3.28...3.31)

project(PBBLinuxUsage
  LANGUAGES C CXX)

find_package(PBB REQUIRED)
add_executable(PBBLinuxUsage main.cxx)
target_link_libraries(PBBLinuxUsage PRIVATE PBB::PBB)
