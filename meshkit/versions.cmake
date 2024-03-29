# This maintains the links for all sources used by this superbuild.
# Simply update this file to change the revision.
# One can use different revision on different platforms.
# e.g.
# if (UNIX)
#   ..
# else (APPLE)
#   ..
# endif()


add_revision(mpich2
  URL "http://www.mcs.anl.gov/research/projects/mpich2/downloads/tarballs/1.4.1p1/mpich2-1.4.1p1.tar.gz"
  URL_MD5 b470666749bcb4a0449a072a18e2c204)

add_revision(freetype
  URL "http://paraview.org/files/dependencies/freetype-2.4.8.tar.gz"
  URL_MD5 "5d82aaa9a4abc0ebbd592783208d9c76")

add_revision(ftgl
  GIT_REPOSITORY "https://github.com/ulrichard/ftgl.git"
  GIT_TAG cf4d9957930e41c3b82a57b20207242c7ef69f18
  )

add_revision(OCE
  GIT_REPOSITORY "https://github.com/robertmaynard/oce.git"
  GIT_TAG "cgm_support"
  )

add_revision(zlib
  URL "http://www.paraview.org/files/v3.12/zlib-1.2.5.tar.gz"
  URL_MD5 c735eab2d659a96e5a594c9e8541ad63)

add_revision(szip
  URL "http://www.hdfgroup.org/ftp/lib-external/szip/2.1/src/szip-2.1.tar.gz"
  URL_MD5 902f831bcefb69c6b635374424acbead)

add_revision(netcdf
  URL "ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-4.3.0.tar.gz"
  URL_MD5 40c0e53433fc5dc59296ee257ff4a813)

add_revision(hdf5
  URL "http://paraview.org/files/dependencies/hdf5-1.8.9.tar.gz"
  URL_MD5 d1266bb7416ef089400a15cc7c963218)

add_revision(moab
  GIT_REPOSITORY https://bitbucket.org/fathomteam/moab.git
  GIT_TAG Version4.7.0
  )

add_revision(cgm
  GIT_REPOSITORY "https://bitbucket.org/fathomteam/cgm.git"
  GIT_TAG 13.1.1
  )

add_revision(lasso
  GIT_REPOSITORY https://bitbucket.org/fathomteam/lasso.git
  GIT_TAG Version3.1
  )

add_revision(meshkit
  GIT_REPOSITORY https://bitbucket.org/fathomteam/meshkit.git
  GIT_TAG MeshKitv1.2
  )
