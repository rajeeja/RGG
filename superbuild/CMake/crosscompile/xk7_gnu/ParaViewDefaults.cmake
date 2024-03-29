set(PARAVIEW_OPTIONS "")
list(APPEND PARAVIEW_OPTIONS "-DBUILD_SHARED_LIBS:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DBUILD_TESTING:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_QT_GUI:BOOL=OFF")
#
list(APPEND PARAVIEW_OPTIONS "-DENABLE_MPI4PY:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_USE_MPI:BOOL=ON")
#/opt/cray/mpt/5.5.5/gni/mpich2-gnu/47/lib/
#libmpich.a 
#libmpichcxx.a 
#libmpichcxx_gnu_47.a
#libmpich_gnu_47.a
list(APPEND PARAVIEW_OPTIONS "-DMPI_INCLUDE_DIRS:STRING=/opt/cray/mpt/5.5.5/gni/mpich2-gnu/47/include")
list(APPEND PARAVIEW_OPTIONS "-DMPI_LIBRARY:FILEPATH=/opt/cray/mpt/5.5.5/gni/mpich2-gnu/47/lib/libmpich.a")
list(APPEND PARAVIEW_OPTIONS "-DMPI_EXTRA_LIBRARY:FILEPATH=/opt/cray/mpt/5.5.5/gni/mpich2-gnu/47/lib/libmpichcxx.a")
list(APPEND PARAVIEW_OPTIONS "-DMPIEXEC:FILEPATH=/sw/xk6/altd/bin/aprun")
#
list(APPEND PARAVIEW_OPTIONS "-DVTK_USE_SYSTEM_HDF5:BOOL=1")
list(APPEND PARAVIEW_OPTIONS "-DHDF5_FOUND:BOOL=1")
list(APPEND PARAVIEW_OPTIONS "-DHDF5_LIBRARIES:STRING=/opt/cray/hdf5-parallel/1.8.7/gnu/46/lib/libhdf5.a")
list(APPEND PARAVIEW_OPTIONS "-DHDF5_INCLUDE_DIRS:STRING=/opt/cray/hdf5-parallel/1.8.7/gnu/46/include")
#
list(APPEND PARAVIEW_OPTIONS "-DVTK_USE_SYSTEM_ZLIB:BOOL=1")
list(APPEND PARAVIEW_OPTIONS "-DZLIB_LIBRARY:FILEPATH=/usr/lib64/libz.a")
list(APPEND PARAVIEW_OPTIONS "-DZLIB_INCLUDE_DIR:PATH=/usr/include")
#
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_ENABLE_FFMPEG:BOOL=OFF")
#
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_USE_VISITBRIDGE:BOOL=ON")
list(APPEND PARAVIEW_OPTIONS "-DVISIT_BUILD_READER_CGNS:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DVISIT_BUILD_READER_Silo:BOOL=OFF")
#
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_AdiosReader:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_AnalyzeNIfTIIO:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_ArrowGlyph:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_EyeDomeLighting:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_ForceTime:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_GMVReader:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_H5PartReader:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_MantaView:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_Moments:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_Nektar:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_NonOrthogonalSource:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_PacMan:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_PointSprite:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_PrismPlugin:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_QuadView:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_SLACTools:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_SciberQuestToolKit:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_SierraPlotTools:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_SurfaceLIC:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_UncertaintyRendering:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_VaporPlugin:BOOL=OFF")
list(APPEND PARAVIEW_OPTIONS "-DPARAVIEW_BUILD_PLUGIN_pvblot:BOOL=OFF")
