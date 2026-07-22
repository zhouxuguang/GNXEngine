#set(GKLIB_PATH "GKlib" CACHE PATH "path to GKlib")
set(GKLIB_PATH "GKlib")

include(${GKLIB_PATH}/GKlibSystem.cmake)
# Add include directories.
include_directories(${GKLIB_PATH})
add_subdirectory("libmetis")
