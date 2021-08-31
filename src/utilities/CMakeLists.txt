#
# Gets variables from root CMakeLists
#

message("in utilities")

set(executables mbabsorption mbdatalist mbgrid mbnavlist mbsegypsd 
    mbareaclean mbdefaults mbhistogram mbpreprocess mbset 
    mbauvloglist mbdumpesf mbinfo mbprocess mbsslayout mbbackangle 
    mbextractsegy mblevitus mbrolltimelag mbsvplist mbclean mbfilter mblist 
    mbroutetime mbsvpselect mbconfig mbformat mbmakeplatform mbsegygrid 
    mbtime mbcopy mbgetesf mbminirovnav mbsegyinfo mbvoxelclean mbctdlist 
    mbgpstide mbmosaic mbsegylist)

foreach(executable ${executables})
  add_executable(${executable}
                 ${executable}.cc)

  target_include_directories(${executable}
                             PUBLIC
                             .
                             ${GMT_INCLUDE_DIR}
                             ${NETCDF_INCLUDE_DIR}                           
                             ${CMAKE_SOURCE_DIR}/src/mbio
                             ${CMAKE_SOURCE_DIR}/src/mbaux
                             ${CMAKE_SOURCE_DIR}/src/gsf)

if (APPLE)
  # Don't know how to build dylib on Apple yet, so link with static libs
  target_link_libraries(${executable}
                        PRIVATE
                        mbio
                        mbbsio
                        mbaux
                        mbsapi
                        mbgsf
	                ${GMT_LIBRARIES}
                        ${NETCDF_LIBRARIES}
                        ${GDAL_LIBRARIES}
                        ${PROJ_LIBRARIES}
                        ${FFTW_LIBRARIES}
                        m
                        pthread)
else()
  target_link_libraries(${executable}
                        PRIVATE
                        mbioShared
                        mbbsioShared
                        mbauxShared
                        mbsapiShared
                        mbgsfShared
	                ${GMT_LIBRARIES}
                        ${NETCDF_LIBRARIES}
                        ${GDAL_LIBRARIES}
                        ${PROJ_LIBRARIES}
                        ${FFTW_LIBRARIES}
                        m
                        pthread)
endif()

endforeach()
                           
                           
