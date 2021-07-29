cmake_minimum_required(VERSION 3.10)

# set the project name
project(SiTech)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

find_package(Nova REQUIRED)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

include(CMakeCommon)

# add the executable
add_executable(SiTech telescope_sitech.cpp)
target_link_libraries(SiTech ${INDI_LIBRARIES} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${NOVA_LIBRARIES})

install(TARGETS SiTech RUNTIME DESTINATION bin)
