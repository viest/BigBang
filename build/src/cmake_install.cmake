# Install script for directory: /home/cch/work/bigbang/core/BigBang-fork-create-test/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/bigbang/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/xengine/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/crypto/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/common/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/mpvss/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/storage/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/network/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/delegate/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/leveldb/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/snappy/cmake_install.cmake")
  include("/home/cch/work/bigbang/core/BigBang-fork-create-test/build/src/jsonrpc/cmake_install.cmake")

endif()

