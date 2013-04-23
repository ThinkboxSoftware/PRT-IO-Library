/**
 * Copyright 2013 Thinkbox Software Inc.
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
 * This file contains I/O functions for working with class any and std::iostreams.
 */
#pragma once

#include <prtio/detail/any.hpp>
#include <prtio/detail/data_types.hpp>

#include <istream>
#include <ostream>
#include <vector>
#include <string>

namespace prtio{ namespace detail{
	/**
	 * Reads an any object from the istream. 
	 * The stream must contain:
	 *  (4 bytes)                           data type 
	 *  (4 bytes)                           arity
	 *  ('sizeof(data type) * arity' bytes) value
	 * \param[out] outVal The any object to populate.
	 * \param istream The strean to read the data from.
	 */
	void read_any( any& outVal, std::istream& istream );
	
	/**
	 * Writes an any object to the ostream.
	 * The stream will contain:
	 *  (4 bytes)                           data type 
	 *  (4 bytes)                           arity
	 *  ('sizeof(data type) * arity' bytes) value
	 * \param val The value to write
	 * \param ostream The stream to write to.
	 */
	void write_any( const any& val, std::ostream& ostream );

	template <typename T>
	struct read_any_impl{
		inline static void apply( any& outVal, std::istream& istream, std::size_t arity ){
			std::vector<T> val( arity );
			
			istream.read( reinterpret_cast<char*>( &val.front() ), sizeof(T) * arity );
			
			outVal.set( std::vector<T>() );
			outVal.get< std::vector<T> >().swap( val );
		}
	};
	
	inline void read_any_string( any& outVal, std::istream& istream, std::size_t arity ){
		std::string stringVal( arity, '\0' );
		istream.read( &stringVal[0], arity );
		
		// Find the first null character and truncate the string to that location.
		std::string::size_type realSize = stringVal.find_first_of( '\0' );
		
		if( realSize != std::string::npos ){
			stringVal.resize( realSize );
			arity = realSize;
		}
		
#ifdef _WIN32
		// We need to convert to std::wstring since 'char' cannot encode UTF-8 on Windows.
		std::wstring wideVal;

		int r = MultiByteToWideChar( CP_UTF8, 0, &stringVal[0], static_cast<int>( arity ), NULL, 0 );
		if( r > 0 ){
			wideVal.assign( static_cast<std::size_t>(r), L'\0' );
			
			r = MultiByteToWideChar( CP_UTF8, 0, &stringVal[0], static_cast<int>( arity ), &wideVal[0], r );
		}
		
		if( r == 0 )
			throw std::runtime_error( "Could not convert metadata value to UTF-16 format" );
		
		outVal.set( std::wstring() );
		outVal.get<std::wstring>().swap( wideVal );
#else				
		// We stick with std::string on non-Windows since it is assumed to encode UTF-8 strings.
		outVal.set( std::string() );
		outVal.get<std::string>().swap( stringVal );
#endif
	}
	
	template <typename T>
	inline void read_any( any& outVal, std::istream& istream, std::size_t arity ){
		read_any_impl<T>::apply( outVal, istream, arity );
	}
	
	inline void read_any( any& outVal, std::istream& istream, int type, std::size_t arity ){
		if( type < -1 || type >= data_types::type_count )
			throw std::runtime_error( "Invalid data type to read from disk" );
		
		if( type == -1 ){
			read_any_string( outVal, istream, arity ); 
		}else{
			switch( type ){
			case data_types::type_int16: 
				read_any<data_types::int16_t>( outVal, istream, arity ); 
				break;
			case data_types::type_int32: 
				read_any<data_types::int32_t>( outVal, istream, arity ); 
				break;
			case data_types::type_int64: 
				read_any<data_types::int64_t>( outVal, istream, arity ); 
				break;
			case data_types::type_uint16: 
				read_any<data_types::uint16_t>( outVal, istream, arity ); 
				break;
			case data_types::type_uint32: 
				read_any<data_types::uint32_t>( outVal, istream, arity ); 
				break;
			case data_types::type_uint64: 
				read_any<data_types::uint64_t>( outVal, istream, arity ); 
				break;
			case data_types::type_float16:
				read_any<data_types::float16_t>( outVal, istream, arity ); 
				break;
			case data_types::type_float32:
				read_any<data_types::float32_t>( outVal, istream, arity ); 
				break;
			case data_types::type_float64:
				read_any<data_types::float64_t>( outVal, istream, arity ); 
				break;
			case data_types::type_int8:
				read_any<data_types::int8_t>( outVal, istream, arity ); 
				break;
			case data_types::type_uint8:
				read_any<data_types::uint8_t>( outVal, istream, arity ); 
				break;
			};
		}
	}
	
	inline void read_any( any& outVal, std::istream& istream ){
		data_types::int32_t type, arity;
		istream.read( reinterpret_cast<char*>( &type ), sizeof(data_types::int32_t) );
		istream.read( reinterpret_cast<char*>( &arity ), sizeof(data_types::int32_t) );
		
		read_any( outVal, istream, type, arity );
	}
	
	inline void write_any( const any& val, std::ostream& ostream ){
		val.write( ostream );
	}

} }
