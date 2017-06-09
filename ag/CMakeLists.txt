# Copyright 2017 Mathieu Bernard
#
# This file is part of wordseg: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# wordseg is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with wordseg. If not, see <http://www.gnu.org/licenses/>.


# Building script for the ag program (Pitman-Yor Context Free Grammar,
# a.k.a Adaptor Grammar)

# looking for -pthread
find_package(Threads)

# looking for boost libraries
find_package(Boost
  REQUIRED
  COMPONENTS program_options date_time filesystem system log thread)

# compiling a C++11 program
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -fext-numeric-literals")


option(AG_QUADRUPLE "compile ag with quadruple precision floating point numbers" OFF)
option(AG_PARALLEL "compile ag with multithreads" OFF)


if(AG_QUADRUPLE)
  add_compile_options("-DQUADPREC")
endif()

message(STATUS "Option AG_QUADRUPLE is ${AG_QUADRUPLE}")


if(AG_PARALLEL)
  find_package(OpenMP REQUIRED)
  add_compile_options(${OpenMP_CXX_FLAGS})
endif()

message(STATUS "Option AG_PARALLEL is ${AG_PARALLEL}")


# the source files to compile
file(GLOB SOURCES src/*.cpp include/*)

# ag binary linked on dependencies
include_directories(${Boost_INCLUDE_DIRS} include)
add_executable(ag ${SOURCES})
target_link_libraries(ag ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
if(AG_PARALLEL)
  target_link_libraries(ag ${OpenMP_CXX_FLAGS})
endif()


# TODO little test from MJ
#
# run-toy: pycfg  testengger.lt testeng.yld
# 	$(EXEC) pycfg -r 1234 -P -D -R -1 -d 100 -a 1e-2 -b 1 -e 1 -f 1 \
# 		-g 1e2 -h 1e-2 -n 10 -C -E -A testeng.prs -N 10 \
# 		-F testeng.trace -G testeng.wlt testengger.lt \
# 		-X "cat > testeng.X1.prs" -X "cat > testeng.X2.prs" \
# 		-u testeng1.yld -U "cat > testeng1.prs" -v testeng2.yld \
# 		-V "cat > testeng2.prs" < testeng.yld
