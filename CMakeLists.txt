# This file is the entry point for configuring/building/installing
# wordseg. From the wordseg top-level directory, create a `build`
# dirdectory and run cmake from there:
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


cmake_minimum_required(VERSION 2.4)


# Project-wide configuration.
#
# Setup project name, authorship, version and build type.

project(wordseg)
set(PROJECT_AUTHOR "Alex Cristia, Mathieu Bernard, Elin Larsen")

# Read the project version from the VERSION file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)

# Set the build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

message(STATUS "Project is wordseg-${PROJECT_VERSION}")
message(STATUS "Setting build type to ${CMAKE_BUILD_TYPE}")


# C++ part.
#
# Compilation of the wordseg algorithms implemented in C++. See the
# CMakeLists.txt in their subdirectories.

add_subdirectory(wordseg/algos/ag)
add_subdirectory(wordseg/algos/dpseg)


# Python part.
#
# Create setup.py from setup.py.in and links 'make wordseg' to a call
# to setup.py.

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


# Documentation part
#
# Build the HTML pages from the doc/ directory, defines the 'make
# html' target

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

  add_custom_target(html DEPENDS ${DOC_BINARY_DIR}/timestamp)

  # dont use CMAKE_INSTALL_PREFIX but use the python path instead
  get_filename_component(DOC_DEST ${PYTHON_EXECUTABLE} DIRECTORY)
  get_filename_component(DOC_DEST ${DOC_DEST} DIRECTORY)
  install(
    DIRECTORY ${DOC_BINARY_DIR}
    DESTINATION ${DOC_DEST}/share/wordseg
    PATTERN "conf.py" EXCLUDE
    PATTERN "timestamp" EXCLUDE)
endif()


# Tests part
#
# Defines the "make test" target and bound it to pytest.


include(CTest)

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
