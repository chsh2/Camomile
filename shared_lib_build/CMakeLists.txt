cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(ekext_lpc VERSION 0.0.1 LANGUAGES C CXX)

list(APPEND LIBPD_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../libpd/pure-data/src")

set(EXTERNAL_COMPILE_DEFINITIONS
    PDTHREADS=1
)
if(NOT DEFINED ENV{PLUGDATA_DIR})
    set(ENV{PLUGDATA_DIR} "C:/Program Files/PlugData")
endif()
link_directories($ENV{PLUGDATA_DIR})

add_library(lpc SHARED ${CMAKE_CURRENT_SOURCE_DIR}/../Source/Pd/ekext/lpc~.c ${CMAKE_CURRENT_SOURCE_DIR}/lpc~.def)
set_target_properties(lpc PROPERTIES OUTPUT_NAME "lpc~")
target_include_directories(lpc PUBLIC ${LIBPD_INCLUDE_DIRECTORY})
target_link_libraries(lpc pd)
target_compile_definitions(lpc PUBLIC ${EXTERNAL_COMPILE_DEFINITIONS})

add_library(lpreson SHARED ${CMAKE_CURRENT_SOURCE_DIR}/../Source/Pd/ekext/lpreson~.c ${CMAKE_CURRENT_SOURCE_DIR}/lpreson~.def)
set_target_properties(lpreson PROPERTIES OUTPUT_NAME "lpreson~")
target_include_directories(lpreson PUBLIC ${LIBPD_INCLUDE_DIRECTORY})
target_link_libraries(lpreson pd)
target_compile_definitions(lpreson PUBLIC ${EXTERNAL_COMPILE_DEFINITIONS})