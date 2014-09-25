#  -*- mode: cmake -*-

#
# Build TPL: Boost 
#

# --- Define all the directories and common external project flags
define_external_project_args(Boost TARGET boost)

# add Boost version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX Boost
  VERSION ${Boost_VERSION_MAJOR} ${Boost_VERSION_MINOR} ${Boost_VERSION_PATCH})

# -- Define build definitions

# We only build what we need, this is NOT a full Boost install
set(Boost_projects "system,filesystem,program_options,regex")

# --- Define the configure command

# Determine toolset type
set(Boost_toolset)
string(TOLOWER ${CMAKE_C_COMPILER_ID} compiler_id_lc)
if (compiler_id_lc)
  if (APPLE)
    # CMAKE_SYSTEM of the form Darwin-12.5.0
    # CMAKE_SYSTEM_VERSION is 12.5.0 corresponds to OSX 10.8.5
    STRING(REGEX REPLACE "\\..*" "" OS_VERSION_MAJOR ${CMAKE_SYSTEM_VERSION})
    #
    if ( ${compiler_id_lc} STREQUAL "intel" )
      set(Boost_toolset intel-darwin)
    else()  
      set(Boost_toolset darwin)
    endif()  
    # In the case that we're building using GCC on Macs, we have to give Boost 
    # some extra hints.
    if (${compiler_id_lc} STREQUAL "gnu")
      # JDM: It seems this only works on OSX 10.9 and above, needs more testing.
      # OSX 10.9.x -> Darwin-13.x.y
      if ( ${OS_VERSION_MAJOR} GREATER 12 )
        message (STATUS "BOOST: build and linking with -stdlib=libstdc++ ")
        set(Boost_bootstrap_args "cxxflags=\"-arch i386 -arch x86_84\" address-model=32_64")
        set(Boost_bjam_args "cxxflags=\"-stdlib=libstdc++\" linkflags=\"-stdlib=libstdc++\"")
      endif()
    endif()
  elseif(UNIX)
    if ( ${compiler_id_lc} STREQUAL "gnu" )
        set(Boost_toolset gcc)
    elseif(${compiler_id_lc} STREQUAL "intel")
        set(Boost_toolset intel-linux)
    elseif(${compiler_id_lc} STREQUAL "pgi")
        set(Boost_toolset pgi)
    elseif(${compiler_id_lc} STREQUAL "pathscale")
        set(Boost_toolset pathscale)
    endif()
  endif()
endif()

configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/boost-configure-step.cmake.in
               ${Boost_prefix_dir}/boost-configure-step.cmake
        @ONLY)
set(Boost_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${Boost_prefix_dir}/boost-configure-step.cmake)

# --- Define the build command

configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/boost-build-step.cmake.in
               ${Boost_prefix_dir}/boost-build-step.cmake
       @ONLY)

set(Boost_BUILD_COMMAND ${CMAKE_COMMAND} -P ${Boost_prefix_dir}/boost-build-step.cmake)     

# --- Add external project build and tie to the ZLIB build target
ExternalProject_Add(${Boost_BUILD_TARGET}
                    DEPENDS   ${Boost_PACKAGE_DEPENDS}             # Package dependency target
                    TMP_DIR   ${Boost_tmp_dir}                     # Temporary files directory
                    STAMP_DIR ${Boost_stamp_dir}                   # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}              # Download directory
                    URL          ${Boost_URL}                      # URL may be a web site OR a local file
                    URL_MD5      ${Boost_MD5_SUM}                  # md5sum of the archive file
                    # -- Configure
                    SOURCE_DIR       ${Boost_source_dir}           # Source directory
                    CONFIGURE_COMMAND ${Boost_CONFIGURE_COMMAND}
                    # -- Build
                    BINARY_DIR        ${Boost_build_dir}           # Build directory 
                    BUILD_COMMAND     ${Boost_BUILD_COMMAND}       # $(MAKE) enables parallel builds through make
                    BUILD_IN_SOURCE   ${Boost_BUILD_IN_SOURCE}     # Flag for in source builds
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}        # Install directory
                    INSTALL_COMMAND  ""
                    # -- Output control
                    ${Boost_logging_args})
