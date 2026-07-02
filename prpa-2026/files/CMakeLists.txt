cmake_minimum_required (VERSION 3.18)
project (PRPA)



find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(benchmark REQUIRED)
find_package(fmt REQUIRED)
find_package(TBB QUIET)
if (NOT TBB_FOUND)
   pkg_check_modules(TBB REQUIRED IMPORTED_TARGET tbb)
   add_library(TBB::tbb ALIAS PkgConfig::TBB)
endif()


pkg_check_modules(GStreamer REQUIRED gstreamer-1.0 IMPORTED_TARGET)
pkg_check_modules(GStreamerVideo REQUIRED gstreamer-video-1.0 IMPORTED_TARGET)


#find_package(SDL2 REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(RESOURCES_DIR ${CMAKE_SOURCE_DIR})
configure_file(src/resources.cpp.in src/resources.cpp)


#add_library(JobQueue OBJECT src/JobQueue.hpp src/JobQueue.cpp)

add_library(stb_image src/stb_image.h src/stb_image.c)
add_library(Otsu src/Otsu.cpp src/logo.c)
target_link_libraries(Otsu PRIVATE Threads::Threads TBB::tbb)

# Part 2: concurrent PrioritySet (PES)
add_library(PrioritySet src/PrioritySet.hpp src/PrioritySet.cpp)
target_link_libraries(PrioritySet PRIVATE Threads::Threads)

add_executable(tests_prioset src/tests_prioset.cpp)
target_link_libraries(tests_prioset PRIVATE PrioritySet Threads::Threads)

add_executable(bench_prioset src/bench_prioset.cpp)
target_link_libraries(bench_prioset PRIVATE PrioritySet benchmark::benchmark Threads::Threads)

# Dependency-free variants (no google-benchmark / conan needed)
add_executable(bench_prioset_lite src/bench_prioset_lite.cpp)
target_link_libraries(bench_prioset_lite PRIVATE PrioritySet Threads::Threads)

add_executable(tests_otsu src/tests_otsu.cpp)
target_link_libraries(tests_otsu PRIVATE Otsu)

add_executable(bench_lite src/bench_lite.cpp)
target_link_libraries(bench_lite PRIVATE Otsu)

add_executable(stream src/stream.cpp src/gstfilter.c src/gstfilter.h)
target_link_libraries(stream PRIVATE PkgConfig::GStreamerVideo Otsu)

add_executable(bench src/bench.cpp  src/resources.cpp)
set_source_files_properties(src/resources.cpp PROPERTIES GENERATED TRUE)


target_link_libraries(bench PRIVATE benchmark::benchmark Otsu stb_image fmt::fmt)
