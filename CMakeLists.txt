# yue.nicholas@gmail.com

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( PRT-IO-Library )

FIND_PACKAGE ( ILMBase )
FIND_PACKAGE ( ZLIB )

INCLUDE_DIRECTORIES ( . )
INCLUDE_DIRECTORIES ( ${ILMBASE_INCLUDE_DIRS} )

ADD_EXECUTABLE ( example example.cpp )
TARGET_LINK_LIBRARIES ( example
  ${Ilmbase_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

ADD_EXECUTABLE ( layout layout.cpp )
TARGET_LINK_LIBRARIES ( layout
  ${Ilmbase_HALF_LIBRARY}
  ${ZLIB_LIBRARY}
)

INSTALL ( FILES
  prtio/detail/conversion.hpp
  prtio/detail/data_types.hpp
  prtio/detail/prt_header.hpp
  prtio/prt_ifstream.hpp
  prtio/prt_istream.hpp
  prtio/prt_layout.hpp
  prtio/prt_ofstream.hpp
  prtio/prt_ostream.hpp
  DESTINATION
  ./include/prtio
)
