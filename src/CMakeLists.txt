cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

set(LIB_FS ${PROJECT_NAME_BASE}-fs)

PROJECT(${LIB_FS} C)
set(CORTXFS_MAJOR_VERSION 1)
set(CORTXFS_MINOR_VERSION 0)
set(CORTXFS_PATCH_LEVEL 1)
set(CORTXFS_EXTRA_VERSION ${RELEASE_VER})

set(DEFAULT_CORTXUTILSINC "")
set(DEFAULT_LIBCORTXUTILS "")

set(CORTXUTILSINC ${DEFAULT_CORTXUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBCORTXUTILS ${DEFAULT_LIBCORTXUTILS} CACHE PATH "Path to folder with libcortx-utils.so")

set(DEFAULT_LIBNSAL "")
set(LIBNSAL ${DEFAULT_LIBNSAL} CACHE PATH "Path to folder with libcortx-nsal.so")

set(DEFAULT_NSALINC "")
set(NSALINC ${DEFAULT_NSALINC} CACHE PATH "Path to folder with kvstore.h")

set(DEFAULT_LIBDSAL "")
set(LIBDSAL ${DEFAULT_LIBDSAL} CACHE PATH "Path to folder with libcortx-dsal.so")

set(DEFAULT_DSALINC "")
set(DSALINC ${DEFAULT_DSALINC} CACHE PATH "Path to folder with dstore.h")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I\"${CMAKE_SOURCE_DIR}/include\"")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC -g")

set(CORTXFS_BASE_VERSION ${BASE_VERSION})
set(INSTALL_DIR_ROOT ${INSTALL_DIR_ROOT})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "cortx-dsal version ${CORTXFS_BASE_VERSION}")

# Option (To enable/disable ENABLE_DASSERT mode.)
option(ENABLE_DASSERT "Enable ENABLE_DASSERT mode." ON)

if (ENABLE_DASSERT)
        set(BCOND_ENABLE_DASSERT "%bcond_without")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_DASSERT")
else (ENABLE_DASSERT)
        set(BCOND_ENABLE_DASSERT "%bcond_with")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif (ENABLE_DASSERT)

message( STATUS "ENABLE_DASSERT : ${ENABLE_DASSERT}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

## Check ini_config
check_include_files("ini_config.h" HAVE_INI_CONFIG_H)
find_library(HAVE_INI_CONFIG ini_config)
check_library_exists(
	ini_config
	config_from_file
	""
	HAVE_INI_CONFIG
	)

if((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))
      message(FATAL_ERROR "Cannot find ini_config")
endif((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))

set(CMAKE_REQUIRED_INCLUDES ${CORTXUTILSINC})

CHECK_INCLUDE_FILES("fault.h" HAVE_CORTX_UTILS_H)

message(STATUS "HAVE_CORTX_UTILS_H=${HAVE_CORTX_UTILS_H}")

if(NOT HAVE_CORTX_UTILS_H)
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find CORTX-UTILS runtime. Disabling CORTXFS build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find CORTX-UTILS runtime. Disabling CORTXFS build")
    set(USE_CORTXFS OFF)
  endif(STRICT_PACKAGE)
endif(NOT HAVE_CORTX_UTILS_H)

include_directories(${CORTXUTILSINC})
link_directories(${LIBCORTXUTILS})

include_directories(${NSALINC})
link_directories(${LIBNSAL})

include_directories(${DSALINC})
link_directories(${LIBDSAL})

add_subdirectory(cortxfs)
set(CORTXFS cortxfs)

add_subdirectory(fs)
set(FS fs)

add_subdirectory(management)
set(MANAGEMENT management)

add_library(${LIB_FS} SHARED
		$<TARGET_OBJECTS:${CORTXFS}>
		$<TARGET_OBJECTS:${FS}>
		$<TARGET_OBJECTS:${MANAGEMENT}>
	)

target_link_libraries(${LIB_FS}
  ini_config
  ${PROJECT_NAME_BASE}-utils
  ${PROJECT_NAME_BASE}-dsal
  ${PROJECT_NAME_BASE}-nsal
  cmocka
)

add_subdirectory(test)

# CPack / rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "${LIB_FS}-Source" )
set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME_BASE}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME_BASE}-FS - ${PROJECT_NAME_BASE} file system")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CORTXFS_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${CORTXFS_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${CORTXFS_PATCH_LEVEL})

# Tell CPack the kind of packages to be generated
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

set(CPACK_SOURCE_IGNORE_FILES
  "/.git/;/.gitignore/;/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")

include(CPack)

set(PKG_NAME "${CPACK_PACKAGE_NAME}.tar.gz")
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# Create the .pc file
configure_file(
  "${PROJECT_SOURCE_DIR}/${LIB_FS}.pc-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_FS}.pc"
)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/${LIB_FS}.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_FS}.spec"
)

set(RPMDEST "--define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}'")

add_custom_target( rpms DEPENDS dist)
add_custom_command(TARGET rpms
                  COMMAND sh -c "rpmbuild ${RPMDEST} -ta ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

add_custom_target( rpm DEPENDS dist)
add_custom_command(TARGET rpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

add_custom_target( srpm DEPENDS dist)
add_custom_command(TARGET srpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} -ts ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

