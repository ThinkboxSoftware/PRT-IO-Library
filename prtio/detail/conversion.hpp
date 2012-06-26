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
 * This file contains classes and functions for querying and converting prt data types at runtime.
 */

#pragma once

#include <cstring>
#include <prtio/detail/data_types.hpp>

namespace prtio{
namespace detail{

	/**
	 * @param type The data type to query
	 * @return True if the given type is floating point, otherwise false.
	 */
	inline bool is_float( data_types::enum_t type ){
		return type >= data_types::type_float16 && type <= data_types::type_float64;
	}

	/**
	 * @param type The data type to query
	 * @return True if the given type is an integer, otherwise false.
	 */
	inline bool is_integral( data_types::enum_t type ){
		return !is_float( type );
	}

	/**
	 * @param type The data type to query
	 * @return True if the given type can be negative, otherwise false.
	 */
	inline bool is_signed( data_types::enum_t type ){
		return type < data_types::type_uint16 || type == data_types::type_int8;
	}

	/**
	 * Determines if a type is comaptible with another, for purposes of converting data. It prevents
	 * conversions that lose information.
	 * @param dest The data type being converted to
	 * @param src The data type being converted from
	 * @return True if the 'src' type can be converted to 'dest' type.
	 */
	inline bool is_compatible( data_types::enum_t dest, data_types::enum_t src ){
		//We allow free conversion between floating point types
		if( is_float( src ) )
			return is_float( dest );

		//Only allow integer conversions that can't lose data.

		if( is_signed(src) )
			return is_signed(dest) && data_types::sizes[dest] >= data_types::sizes[src];

		//We allow a unsigned -> signed conversion if the bit depth increases.
		return is_signed(dest) ? data_types::sizes[dest] > data_types::sizes[src] : data_types::sizes[dest] >= data_types::sizes[src];
	}

	//This typedef is for holding a function pointer to prt_converter<T1,T2>::apply. It is used
	//for converting and copying data at runtime.
	typedef void(*convert_fn_t)(void*, const void*, std::size_t);

	/**
	 * This template class provides a static function that will convert data from one type
	 * to another.
	 * @tparam TDest The type to convert to
	 * @tparam TSrc The type to convert from
	 */
	template <typename TDest, typename TSrc>
	struct prt_converter{
		/**
		 * This function is used to convert data from one type to another at runtime.
		 * @param dest A pointer to the destination data.
		 * @param src A pointer to the source data.
		 * @param arity The number of consectuive elements to process
		 */
		static void apply( void* dest, const void* src, std::size_t arity ){
			for( std::size_t i = 0; i < arity; ++i )
				reinterpret_cast<TDest*>( dest )[i] = static_cast<TDest>( reinterpret_cast<const TSrc*>( src )[i] );
		}
	};

	/**
	 * This template specialization is for when no actual type conversion is taking place. It uses
	 * memcpy instead.
	 * @overload
	 */
	template <typename T>
	struct prt_converter<T, T>{
		static void apply( void* dest, const void* src, std::size_t arity ){
			memcpy( dest, src, sizeof(T) * arity );
		}
	};

	/**
	 * This template specialization does an extra cast to avoid compiler errors (since half can only be converted
	 * to float).
	 * @overload
	 */
	template <typename TDest>
	struct prt_converter<TDest, half>{
		static void apply( void* dest, const void* src, std::size_t arity ){
			for( std::size_t i = 0; i < arity; ++i )
				reinterpret_cast<TDest*>( dest )[i] = static_cast<TDest>( static_cast<float>( reinterpret_cast<const half*>( src )[i] ) );
		}
	};

	/**
	 * This template specialization does an extra cast to avoid compiler errors (since half can only be converted
	 * to float).
	 * @overload
	 */
	template <typename TSrc>
	struct prt_converter<half, TSrc>{
		static void apply( void* dest, const void* src, std::size_t arity ){
			for( std::size_t i = 0; i < arity; ++i )
				reinterpret_cast<half*>( dest )[i] = static_cast<float>( reinterpret_cast<const TSrc*>( src )[i] );
		}
	};

	/**
	 * This template specialization is required fix ambiguous partial specializations on half.
	 * @overload
	 */
	template <>
	struct prt_converter<half, half>{
		static void apply( void* dest, const void* src, std::size_t arity ){
			memcpy( dest, src, sizeof(half) * arity );
		}
	};

	/**
	 * This template function is used for getting a converter to a compile-time known type,
	 * when the source type is only known at runtime.
	 * @tparam TDest The C++ type to convert to.
	 * @param srcType The PRT IO type of the source data.
	 * @return A function pointer to a function that converts from 'srcType' to TDest. NULL if 'srcType' is not known.
	 */
	template <class TDest>
	convert_fn_t get_read_converter( data_types::enum_t srcType ){
		switch( srcType ){
		case data_types::type_int8:
			return &prt_converter<TDest, data_types::int8_t>::apply;
		case data_types::type_int16:
			return &prt_converter<TDest, data_types::int16_t>::apply;
		case data_types::type_int32:
			return &prt_converter<TDest, data_types::int32_t>::apply;
		case data_types::type_int64:
			return &prt_converter<TDest, data_types::int64_t>::apply;
		case data_types::type_float16:
			return &prt_converter<TDest, data_types::float16_t>::apply;
		case data_types::type_float32:
			return &prt_converter<TDest, data_types::float32_t>::apply;
		case data_types::type_float64:
			return &prt_converter<TDest, data_types::float64_t>::apply;
		case data_types::type_uint8:
			return &prt_converter<TDest, data_types::uint8_t>::apply;
		case data_types::type_uint16:
			return &prt_converter<TDest, data_types::uint16_t>::apply;
		case data_types::type_uint32:
			return &prt_converter<TDest, data_types::uint32_t>::apply;
		case data_types::type_uint64:
			return &prt_converter<TDest, data_types::uint64_t>::apply;
		default:
			return NULL;
		}
	}

	/**
	 * This template function is used for getting a converter from a compile-time known type,
	 * when the destination type is only known at runtime.
	 * @tparam TSrc The C++ type to convert from.
	 * @param destType The PRT IO type of the destination data.
	 * @return A function pointer to a function that converts from TSrc to 'destType'. NULL if 'destType' is not known.
	 */
	template <class TSrc>
	convert_fn_t get_write_converter( data_types::enum_t destType ){
		switch( destType ){
		case data_types::type_int8:
			return &prt_converter<data_types::int8_t, TSrc>::apply;
		case data_types::type_int16:
			return &prt_converter<data_types::int16_t, TSrc>::apply;
		case data_types::type_int32:
			return &prt_converter<data_types::int32_t, TSrc>::apply;
		case data_types::type_int64:
			return &prt_converter<data_types::int64_t, TSrc>::apply;
		case data_types::type_float16:
			return &prt_converter<data_types::float16_t, TSrc>::apply;
		case data_types::type_float32:
			return &prt_converter<data_types::float32_t, TSrc>::apply;
		case data_types::type_float64:
			return &prt_converter<data_types::float64_t, TSrc>::apply;
		case data_types::type_uint8:
			return &prt_converter<data_types::uint8_t, TSrc>::apply;
		case data_types::type_uint16:
			return &prt_converter<data_types::uint16_t, TSrc>::apply;
		case data_types::type_uint32:
			return &prt_converter<data_types::uint32_t, TSrc>::apply;
		case data_types::type_uint64:
			return &prt_converter<data_types::uint64_t, TSrc>::apply;
		default:
			return NULL;
		}
	}

}//namespace detail
}//namespace prtio