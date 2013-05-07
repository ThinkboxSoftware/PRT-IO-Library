# yue.nicholas@gmail.com

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( PRT-IO-Library )

FIND_PACKAGE ( ILMBase )
FIND_PACKAGE ( ZLIB )

INCLUDE_DIRECTORIES ( ../ )
INCLUDE_DIRECTORIES ( ${ILMBASE_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES ( ${ZLIB_INCLUDE_DIRS} )

ADD_EXECUTABLE ( example ../example.cpp )
TARGET_LINK_LIBRARIES ( example
  ${Ilmbase_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

ADD_EXECUTABLE ( example_layout ../example_layout.cpp )
TARGET_LINK_LIBRARIES ( example_layout
  ${Ilmbase_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

ADD_EXECUTABLE ( example_metadata ../example_metadata.cpp )
TARGET_LINK_LIBRARIES ( example_metadata
  ${Ilmbase_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

INSTALL ( DIRECTORY
  prtio
  DESTINATION
  ../include
)
