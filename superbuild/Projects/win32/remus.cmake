
add_external_project(remus
  DEPENDS boost zeroMQ
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS=OFF
    -DRemus_ENABLE_EXAMPLES:BOOL=OFF
    -DRemus_NO_SYSTEM_BOOST:BOOL=ON
    -DZeroMQ_ROOT_DIR=<INSTALL_DIR>
    -DBOOST_INCLUDEDIR=<INSTALL_DIR>/include/boost
    -DBOOST_LIBRARYDIR=<INSTALL_DIR>/lib
  PATCH_COMMAND ${GIT_EXECUTABLE} apply
                ${SuperBuild_PROJECTS_DIR}/patches/remus.fix_windows_building.patch
  )
