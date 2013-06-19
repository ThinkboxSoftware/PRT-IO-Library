# yue.nicholas@gmail.com

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( PRT-IO-Library )

# We can place facility-specific Find<BlahBlah>.cmake files in this subdirectory. Mostly used
# to specify package locations on Windows builds since there isn't a known place to look.
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/Modules/")

FIND_PACKAGE ( ILMBase )
FIND_PACKAGE ( ZLIB )

INCLUDE_DIRECTORIES ( ../ )
INCLUDE_DIRECTORIES ( ${ILMBASE_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES ( ${ZLIB_INCLUDE_DIRS} )

if ( MSVC )
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /Zi")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
endif()

ADD_EXECUTABLE ( example ../example.cpp )
TARGET_LINK_LIBRARIES ( example
  ${ILMBASE_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

ADD_EXECUTABLE ( example_layout ../example_layout.cpp )
TARGET_LINK_LIBRARIES ( example_layout
  ${ILMBASE_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

ADD_EXECUTABLE ( example_metadata ../example_metadata.cpp )
TARGET_LINK_LIBRARIES ( example_metadata
  ${ILMBASE_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

INSTALL ( DIRECTORY
  ../prtio
  DESTINATION
  ./include
)
