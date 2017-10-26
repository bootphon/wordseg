# Building script for the ag program (Pitman-Yor Context Free Grammar,
# a.k.a Adaptor Grammar)

# to avoid a warning (cmake complains for a mix of relative and
# absolute paths for linked libraries)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# looking for -pthread
find_package(Threads)

# looking for boost libraries
find_package(Boost REQUIRED COMPONENTS program_options)

# compiling a C++11 program
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")


option(AG_QUADRUPLE "compile ag with quadruple precision floating point numbers" OFF)
if(AG_QUADRUPLE)
  add_compile_options("-DQUADPREC")
endif()
message(STATUS "Option AG_QUADRUPLE is ${AG_QUADRUPLE}")


# The AG_PARALLEL options seems to slow down the execution on a
# machine with less than 8 cores available, because the python wrapper
# already run 8 parallel subprocesses of the original AG code.  Notes
# by Mark Johnson executing the original code: On my 8 core desktop
# machine, the multi-threaded version runs about twice as fast as the
# single threaded version, albeit using on average about 6 cores
# (i.e., its parallel efficiency is about 33%).
option(AG_PARALLEL "compile ag with multithreads support" OFF)
if(AG_PARALLEL)
  find_package(OpenMP REQUIRED)
  add_compile_options(${OpenMP_CXX_FLAGS})
endif()
message(STATUS "Option AG_PARALLEL is ${AG_PARALLEL}")


# the source files to compile
file(GLOB SOURCES src/*.cc include/*)

# ag binary linked on dependencies
include_directories(include ${Boost_INCLUDE_DIRS})
add_executable(ag ${SOURCES})
target_link_libraries(ag ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
if(AG_PARALLEL)
  target_link_libraries(ag ${OpenMP_CXX_FLAGS})
endif()
