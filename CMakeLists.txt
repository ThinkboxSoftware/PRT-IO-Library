# yue.nicholas@gmail.com

cmake_minimum_required(VERSION 3.12)

project(PRT-IO-Library)

# We can place facility-specific Find<BlahBlah>.cmake files in this subdirectory. Mostly used
# to specify package locations on Windows builds since there isn't a known place to look.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/Modules/")

find_package(IlmBase REQUIRED CONFIG)
find_package(ZLIB)

include_directories(.)
# INCLUDE_DIRECTORIES ( ${ILMBASE_INCLUDE_DIRS} )
# INCLUDE_DIRECTORIES ( ${ZLIB_INCLUDE_DIRS} )

set(REQUIRED_LINK_LIBRARIES
  IlmBase::IlmBaseConfig
  IlmBase::Half
  ${ZLIB_LIBRARY}
  )
if ( MSVC )
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /Zi")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
endif()

add_executable(example example.cpp)
target_link_libraries(example
  ${REQUIRED_LINK_LIBRARIES}
)

add_executable(example_layout example_layout.cpp)
target_link_libraries(example_layout
  ${REQUIRED_LINK_LIBRARIES}
)

add_executable(example_metadata example_metadata.cpp)
target_link_libraries(example_metadata
  ${REQUIRED_LINK_LIBRARIES}
)

install(DIRECTORY
  prtio
  DESTINATION
  include
)
