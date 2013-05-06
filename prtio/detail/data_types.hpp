/**
 * Copyright 2012 Thinkbox Software Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file contains definitions for working with prt data types at runtime and compile time.
 * See http://www.thinkboxsoftware.com/krak-prt-file-format/ for more details.
 */

#pragma once

#include <string>
#include <stdexcept>

#include <string>

#pragma warning( push, 3 )
#include <half.h>
#pragma warning( pop )

#include <cctype>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#if _MSC_VER >= 1600
#define HAS_CSTDINT_TYPES
#define STDNAMESPACE std
#include <cstdint>
#endif
#else
#define HAS_CSTDINT_TYPES
#define STDNAMESPACE
#include <stdint.h>
#endif

namespace prtio{
namespace data_types{

	//This list contains all the types defined in the PRT file spec at
	//http://www.thinkboxsoftware.com/krak-prt-file-format/
	enum enum_t{
		type_int16,
		type_int32,
		type_int64,
		type_float16,
		type_float32,
		type_float64,
		type_uint16,
		type_uint32,
		type_uint64,
		type_int8,
		type_uint8,
		type_count //This must be the last entry. It's a marker for the number of possible inputs
	};

	typedef half   float16_t;
	typedef float  float32_t;
	typedef double float64_t;

//I foresee this changing depending on the platform & compiler.
#ifndef HAS_CSTDINT_TYPES
	typedef char      int8_t;
	typedef short     int16_t;
	typedef int       int32_t;
	typedef long long int64_t;

	typedef unsigned char      uint8_t;
	typedef unsigned short     uint16_t;
	typedef unsigned int       uint32_t;
	typedef unsigned long long uint64_t;
#else
	using STDNAMESPACE::int8_t;
	using STDNAMESPACE::int16_t;
	using STDNAMESPACE::int32_t;
	using STDNAMESPACE::int64_t;

	using STDNAMESPACE::uint8_t;
	using STDNAMESPACE::uint16_t;
	using STDNAMESPACE::uint32_t;
	using STDNAMESPACE::uint64_t;
#endif

	//This global array may be indexed using the corresponding type enumeration. For example, the size of
	//a float32 is sizes[type_float32].
	const std::size_t sizes[] = {
		sizeof(int16_t), sizeof(int32_t), sizeof(int64_t),
		sizeof(float16_t), sizeof(float32_t), sizeof(float64_t),
		sizeof(uint16_t), sizeof(uint32_t), sizeof(uint64_t),
		sizeof(int8_t), sizeof(uint8_t)
	};

	//This global array maps from enum_t values to names. For example the name for type_float32 is names[type_float32].
	const char* names[] = {
		"int16", "int32", "int64",
		"float16", "float32", "float64",
		"uint16", "uint32", "uint64",
		"int8", "uint8"
	};

	inline bool is_float( enum_t dt ){ return type_float16 >= dt && type_float64 <= dt; }
	
	/**
	 * The traits template class and it specializations exist to provide compile time information about a mapping
	 * from C++ types to PRT io types.
	 * @tparam T The type we are mapping into the PRT file io enumeration types.
	 * @note traits<T>::data_type() returns the enum_t value of the equivalent PRT io type for T.
	 */
	template <typename T>
	struct traits;

#define PRT_TRAITS_IMPL( type, enumVal ) \
	template <> \
	struct traits< type >{ \
		inline static enum_t data_type(){ \
			return enumVal; \
		} \
	};

	PRT_TRAITS_IMPL( int8_t , type_int8 );
	PRT_TRAITS_IMPL( int16_t, type_int16 );
	PRT_TRAITS_IMPL( int32_t, type_int32 );
	PRT_TRAITS_IMPL( int64_t, type_int64 );

	PRT_TRAITS_IMPL( uint8_t , type_uint8 );
	PRT_TRAITS_IMPL( uint16_t, type_uint16 );
	PRT_TRAITS_IMPL( uint32_t, type_uint32 );
	PRT_TRAITS_IMPL( uint64_t, type_uint64 );

	PRT_TRAITS_IMPL( float16_t, type_float16 );
	PRT_TRAITS_IMPL( float32_t, type_float32 );
	PRT_TRAITS_IMPL( float64_t, type_float64 );

#if defined(_WIN32) || defined(_WIN64)
	//Windows treats long and int as separate types.
	PRT_TRAITS_IMPL( long, type_int32 );
	PRT_TRAITS_IMPL( unsigned long, type_uint32 );
#endif

#undef PRT_TRAITS_IMPL
	
	/**
	 * Extracts a data_type::enum_t from a string representation.
	 */
	std::pair<enum_t,std::size_t> parse_data_type( const std::string& typeString ){
		std::string::const_iterator it = typeString.begin(), itEnd = typeString.end();

		//Skip beginning whitespace
		for( ; it != itEnd && std::isspace(*it); ++it )
			;

		//The type comes first, ending at whitespace or a [ bracket.
		std::string::const_iterator typeStart = it;
		for( ; it != itEnd && !std::isspace(*it) && (*it) != '['; ++it )
			;
		std::string::const_iterator typeEnd = it;

		if( it == itEnd )
			throw std::runtime_error( "Invalid data type string: \"" + typeString + "\"" );

		//Skip whitespace until an open bracket.
		for( ; it != itEnd && std::isspace(*it); ++it )
			;

		//Make sure we closed the arity in brackets correctly
		if( it == itEnd || (*it) != '[' )
			throw std::runtime_error( "Invalid data type string: \"" + typeString + "\"" );

		std::string::const_iterator arityStart = ++it;
		for( ; it != itEnd && std::isdigit(*it); ++it )
			;

		if( it == itEnd || (*it) != ']' || ++it != itEnd )
			throw std::runtime_error( "Invalid data type string: \"" + typeString + "\"" );

		enum_t resultType = type_count;
		std::size_t resultArity = 0;

		for( int i = 0, iEnd = type_count; i < iEnd && resultType == type_count; ++i ){
			if( std::strncmp( names[i], &*typeStart, (typeEnd - typeStart) ) == 0 )
				resultType = static_cast<enum_t>( i );
		}

		resultArity = static_cast<std::size_t>( atoi( &*arityStart ) );

		return std::make_pair( resultType, resultArity );
	}

}//namespace data_types
}//namespace prtio
