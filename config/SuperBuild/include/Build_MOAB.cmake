#  -*- mode: cmake -*-

#
# Build TPL: MOAB 
# 

# --- Define all the directories and common external project flags
define_external_project_args(MOAB 
                             TARGET moab
                             DEPENDS ZLIB HDF5 NetCDF)

# add version version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
                         PREFIX MOAB
                         VERSION ${MOAB_VERSION_MAJOR} ${MOAB_VERSION_MINOR} ${MOAB_VERSION_PATCH})

# --- Build common compiler and link flags
# Build compiler flag strings for C
include(BuildWhitespaceString)
build_whitespace_string(moab_cflags -I${TPL_INSTALL_PREFIX}/include ${Amanzi_COMMON_CFLAGS})

build_whitespace_string(moab_cxxflags -I${TPL_INSTALL_PREFIX}/include ${Amanzi_COMMON_CXXFLAGS})

# Build the LDFLAGS string      
if (BUILD_SHARED_LIBS)
  set(moab_shared "yes")
  set(moab_static "no")
  build_whitespace_string(moab_shared_dir "-Wl,-rpath -Wl,${TPL_INSTALL_PREFIX}/lib")
else()
  set(moab_shared "no")
  set(moab_static "yes")
endif()

build_whitespace_string(moab_ldflags
                        -L${TPL_INSTALL_PREFIX}/lib
                        -L${TPL_INSTALL_PREFIX}/lib
                        -lnetcdf
                        -L${TPL_INSTALL_PREFIX}/lib
                        -lhdf5_hl
                        -lhdf5
                        -L${TPL_INSTALL_PREFIX}/lib
                        -lz
                        ${moab_shared_dir})

# --- Patch the original code
set(MOAB_patch_file moab-install.patch)
set(MOAB_sh_patch ${MOAB_prefix_dir}/moab-patch-step.sh)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/moab-patch-step.sh.in
               ${MOAB_sh_patch}
               @ONLY)
# configure the CMake patch step
set(MOAB_cmake_patch ${MOAB_prefix_dir}/moab-patch-step.cmake)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/moab-patch-step.cmake.in
               ${MOAB_cmake_patch}
               @ONLY)
# set the patch command
set(MOAB_PATCH_COMMAND ${CMAKE_COMMAND} -P ${MOAB_cmake_patch})

# --- Define the arguments passed to CMake.
set(MOAB_CMAKE_CACHE_ARGS 
      "-DCMAKE_INSTALL_PREFIX:FILEPATH=${TPL_INSTALL_PREFIX}")

# --- Add external project build and tie to the SuperLU build target
ExternalProject_Add(${MOAB_BUILD_TARGET}
                    DEPENDS   ${MOAB_PACKAGE_DEPENDS}    # Package dependency target
                    TMP_DIR   ${MOAB_tmp_dir}            # Temporary files directory
                    STAMP_DIR ${MOAB_stamp_dir}          # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}     
                    URL          ${MOAB_URL}             # URL may be a web site OR a local file
                    URL_MD5      ${MOAB_MD5_SUM}         # md5sum of the archive file
                    # -- Patch 
                    PATCH_COMMAND ${MOAB_PATCH_COMMAND}  # Modifications to source
                    # -- Configure
                    SOURCE_DIR   ${MOAB_source_dir}      # Source directory
                    CMAKE_CACHE_ARGS ${AMANZI_CMAKE_CACHE_ARGS}  # Global definitions from root CMakeList
                                     ${MOAB_CMAKE_CACHE_ARGS}
                                     -DCMAKE_C_FLAGS:STRING=${Amanzi_COMMON_CFLAGS}  # Ensure uniform build
                                     -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                     -DCMAKE_CXX_FLAGS:STRING=${Amanzi_COMMON_CXXFLAGS}
                                     -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                                     -DENABLE_FORTRAN:BOOL=TRUE
                                     -DENABLE_MPI:BOOL=TRUE
                                     -DMPI_CXX_COMPILER:FILEPATH=${MPI_CXX_COMPILER}
                                     -DMPI_C_COMPILER:FILEPATH=${MPI_C_COMPILER}
                                     -DENABLE_HDF5:BOOL=TRUE
                                     -DHDF5_ROOT:FILEPATH=${TPL_INSTALL_PREFIX}
                                     -DENABLE_NETCDF:BOOL=TRUE
                                     -DNETCDF_ROOT:FILEPATH=${TPL_INSTALL_PREFIX}
                                     -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}

                    # -- Build
                    BINARY_DIR       ${MOAB_build_dir}         # Build directory 
                    BUILD_COMMAND    ${MAKE} 
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}     # Install directory
                INSTALL_COMMAND  $(MAKE) install
                    # -- Output control
                    ${MOAB_logging_args})

