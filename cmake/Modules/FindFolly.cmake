## = Usage: ============================================================================================================
## FIND_PACKAGE(Folly)    -- (after adding the location to the CMAKE_MODULE_PATH)
## =====================================================================================================================
## = Features: =========================================================================================================
## - Find the headers and library of Folly (Facebook Open-source Library)
## =====================================================================================================================
## = Further Settings: =================================================================================================
## - SPEC_Folly_INCLUDE_DIR: The include directory of Folly if the module has problems finding the proper include path.
## - SPEC_Folly_LIBRARY_DIR: The directory containing Folly if the module has problems finding the proper library.
## =====================================================================================================================
## = Postconditions: ===================================================================================================
## - Folly_FOUND:       System has library and headers of Folly.
## - Folly_INCLUDE_DIR: The Folly's include directory.
## - Folly_LIBRARY:     The Folly library.
## - Folly_VERSION:     The version of the installed Folly library.
## - Folly::Folly:      Target that can be used to link the Folly library using:
##                      TARGET_LINK_LIBRARY(<target> Folly::Folly).
## =====================================================================================================================

INCLUDE(FindPackageHandleStandardArgs)

# Find the include directory of Folly:
FIND_PATH(Folly_INCLUDE_DIR
          NAMES MPMCQueue.h
          HINTS ${SPEC_Folly_INCLUDE_DIR}
          PATH_SUFFIXES include/folly
          DOC "The Folly include directory."
         )

# Find the Folly library:
FIND_LIBRARY(Folly_LIBRARY
             NAMES folly
             HINTS ${SPEC_Folly_LIBRARY_DIR}
             DOC "The Folly library."
            )

# Find the version of Folly:
IF(Folly_INCLUDE_DIR)
    FILE(READ "${Folly_INCLUDE_DIR}/folly-config.h" Folly_H_FILE)
    STRING(REGEX REPLACE ".*#define[ \t]+FOLLY_VERSION[ \t]+\"([0-9\\.]+)\".*" "\\1" Folly_VERSION "${Folly_H_FILE}")
    UNSET(Folly_H_FILE)
ENDIF(Folly_INCLUDE_DIR)

# Handle the REQUIRED/QUIET/version options of FIND_PACKAGE and set Folly_FOUND:
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Folly
                                  REQUIRED_VARS Folly_INCLUDE_DIR Folly_LIBRARY
                                  VERSION_VAR Folly_VERSION
                                 )

# Create the target Folly::Folly:
IF(Folly_FOUND AND NOT TARGET Folly::Folly)
    ADD_LIBRARY(Folly::Folly UNKNOWN IMPORTED)
    SET_TARGET_PROPERTIES(Folly::Folly PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Folly_INCLUDE_DIR}")
    SET_PROPERTY(TARGET Folly::Folly APPEND PROPERTY IMPORTED_LOCATION "${Folly_LIBRARY}")
ENDIF(Folly_FOUND AND NOT TARGET Folly::Folly)

MARK_AS_ADVANCED(Folly_INCLUDE_DIR
                 Folly_LIBRARY
                 Folly_VERSION
                )
