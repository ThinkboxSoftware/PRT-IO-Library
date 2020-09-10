# yue.nicholas@gmail.com

cmake_minimum_required(VERSION 3.12)

project(PRT-IO-Library)

# We can place facility-specific Find<BlahBlah>.cmake files in this subdirectory. Mostly used
# to specify package locations on Windows builds since there isn't a known place to look.
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/Modules/")

if(USE_HOUDINI)
  find_package(HDK REQUIRED)
  include_directories(${HDK_INCLUDE_DIR})
  link_directories($ENV{HFS}/custom/houdini/dsolib)
else()
  find_package(IlmBase REQUIRED CONFIG)
  find_package(ZLIB)
endif()

include_directories(.)
# INCLUDE_DIRECTORIES ( ${ILMBASE_INCLUDE_DIRS} )
# INCLUDE_DIRECTORIES ( ${ZLIB_INCLUDE_DIRS} )

if(USE_HOUDINI)
  if(MSVC)
	set(ZLIB_LIBRARY zdll)
  else()
	set(ZLIB_LIBRARY z)
  endif()
  set(REQUIRED_LINK_LIBRARIES
    Half
    ${ZLIB_LIBRARY}
    )
else()
  set(REQUIRED_LINK_LIBRARIES
    IlmBase::IlmBaseConfig
    IlmBase::Half
    ${ZLIB_LIBRARY}
    )
endif()

if ( MSVC )
  if(USE_HOUDINI)
	add_definitions(-DHALF_EXPORTS -DOPENEXR_DLL)
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /Zi")
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
