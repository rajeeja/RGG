cmake_minimum_required(VERSION 2.8.8)

if (APPLE)
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    message(WARNING "Ensure that CMAKE_OSX_SYSROOT, CMAKE_OSX_DEPLOYMENT_TARGET are set correctly")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "By default, build for 64-bit Leopard")
  endif()
endif()


project(MeshKitSuperBuild)
include(CMakeDependentOption)

set (platform)
if (APPLE)
  set (platform "apple")
elseif (UNIX)
  set (platform "unix")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
  set (cflags "${cflags} -fPIC")
  set (cppflags "${cppflags} -fPIC")
  set (cxxflags "${cxxflags} -fPIC")
elseif (WIN32)
  set (platform "win32")
endif()

# Setup CMAKE_MODULE_PATH so that platform specific configurations are processed
# before the generic ones.
set (CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/Projects/${platform}"
  "${CMAKE_CURRENT_SOURCE_DIR}/Projects/"
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake"
  ${CMAKE_MODULE_PATH}
  )


set (MeshKitSuperBuild_CMAKE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include (MeshKitModules)
include (${CMAKE_CURRENT_SOURCE_DIR}/versions.cmake)

set (install_location "${CMAKE_CURRENT_BINARY_DIR}/install")
set (download_location "${CMAKE_CURRENT_BINARY_DIR}/downloads")
set (patch_location "${CMAKE_CURRENT_SOURCE_DIR}/patches")
set (prefix_path "${install_location}")

option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )

find_package(Autotools REQUIRED)

if(BUILD_WITH_CUBIT)
  set(CUBIT_PATH CACHE PATH "Location of the CUBIT Libraries")
  if(NOT IS_DIRECTORY ${CUBIT_PATH})
    message(FATAL_ERROR "CUBIT_PATH needs to be set to a valid path")
  endif()
endif()

if (APPLE)
  set (cflags "${cflags} -arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (cppflags "${cppflags} -arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (cxxflags "${cxxflags} -arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  if(BUILD_WITH_CUBIT)
    if(${CMAKE_OSX_ARCHITECTURES} STREQUAL "x86_64")
      message(FATAL_ERROR "For CUBIT Builds CMAKE_OSX_ARCHITECTURES needs to be set to i386")
    endif()

  endif()
endif()

#currently remove mpich2 support to make it easier to build for now
#add_project(mpich2)


add_project(zlib SYSTEM REQUIRED)
add_project(szip SYSTEM REQUIRED)
add_project(hdf5 SYSTEM REQUIRED)
include(netcdf)
if(NOT BUILD_WITH_CUBIT)
  add_project(freetype SYSTEM REQUIRED)
  add_project(ftgl SYSTEM REQUIRED)
  add_project(OCE SYSTEM REQUIRED)
endif()
add_project(cgm REQUIRED)
add_project(moab REQUIRED)
add_project(lasso REQUIRED)
add_project(meshkit REQUIRED)


INSTALL( DIRECTORY ${CMAKE_BINARY_DIR}/install/bin USE_SOURCE_PERMISSIONS DESTINATION meshkit )
INSTALL( DIRECTORY ${CMAKE_BINARY_DIR}/install/lib USE_SOURCE_PERMISSIONS DESTINATION meshkit )
