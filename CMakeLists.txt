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


# This file is the entry point for configuring/building/installing
# wordseg. From the wordseg top-level directory, create a `build` dir
# and run cmake from there:
#
#   mkdir ./build
#   cd ./build
#   cmake ..
#
# You have now configured the project and generated a Makefile. Then
# build and install it with:
#
#   make
#   [sudo] make install


cmake_minimum_required(VERSION 3.0)


#
# project-wide configuration
#

# define the project, authors and its version from the VERSION file
project(wordseg)
set(PROJECT_AUTHOR "Alex Cristia, Mathieu Bernard, Elin Larsen")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
message(STATUS "Project is wordseg-${PROJECT_VERSION}")

# enable "make test"
include(CTest)

# Set the build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()
message(STATUS "Setting build type to ${CMAKE_BUILD_TYPE}")


#
# C++ part
#

# make ag
add_subdirectory(ag)

# make dpseg
add_subdirectory(dpseg)


#
# Python part
#

find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)
message(STATUS "Python is ${PYTHON_EXECUTABLE}")

# configure setup.py
set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
configure_file(${SETUP_PY_IN} ${SETUP_PY})

# get the python source files
file(GLOB PROJECT_PYTHON_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/wordseg/*.py
  ${CMAKE_CURRENT_SOURCE_DIR}/wordseg/algos/*.py)

# make wordseg -> python setup.py build
add_custom_target(wordseg ALL
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build -b ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ag dpseg
  SOURCES ${PROJECT_PYTHON_SOURCES})


# make install -> python setup.py install
install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")

# make develop -> python setup.py develop
add_custom_target(develop
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} develop
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  DEPENDS wordseg)

#
# Documentation part
#

# make doc
find_program(PYTHON_SPHINX "sphinx-build")
if(PYTHON_SPHINX)
  message(STATUS "Found Sphinx as ${PYTHON_SPHINX}")

  set(DOC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/doc)
  set(DOC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

  set(CONF_PY_IN "${DOC_SOURCE_DIR}/conf.py.in")
  set(CONF_PY "${DOC_BINARY_DIR}/conf.py")
  configure_file(${CONF_PY_IN} ${CONF_PY})

  file(GLOB DOC_SOURCES ${DOC_SOURCE_DIR}/*.rst)
  add_custom_command(
    OUTPUT ${DOC_BINARY_DIR}/timestamp
    COMMAND ${PYTHON_SPHINX} -b html -c ${DOC_BINARY_DIR} ${DOC_SOURCE_DIR} ${DOC_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${DOC_BINARY_DIR}/timestamp
    DEPENDS ${DOC_SOURCES})

  add_custom_target(doc DEPENDS ${DOC_BINARY_DIR}/timestamp)

  # dont use CMAKE_INSTALL_PREFIX but use the python path instead
  get_filename_component(DOC_DEST ${PYTHON_EXECUTABLE} DIRECTORY)
  get_filename_component(DOC_DEST ${DOC_DEST} DIRECTORY)
  install(
    DIRECTORY ${DOC_BINARY_DIR}
    DESTINATION ${DOC_DEST}/share/wordseg
    PATTERN "conf.py" EXCLUDE
    PATTERN "timestamp" EXCLUDE)
endif()


#
# Tests part
#

# make test
if(BUILD_TESTING)
  find_program(PYTHON_PYTEST "pytest" REQUIRED)
  set(CTEST_BUILD_COMMAND "${CTEST_BUILD_COMMAND} -V")
  file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test/test*.py)
  foreach(SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME "${SOURCE}" NAME)
    add_test(NAME wordseg.${TEST_NAME}
      COMMAND ${PYTHON_PYTEST} ${SOURCE} -v)
  endforeach()
endif()
