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
 * This file contains the interface for reading streams of prt data.
 */

#pragma once

#include <prtio/detail/conversion.hpp>
#include <prtio/detail/data_types.hpp>
#include <prtio/detail/any.hpp>
#include <prtio/prt_layout.hpp>

#include <cstring>
#include <exception>
#include <string>
#include <sstream>
#include <vector>

namespace prtio{

/**
 * This class defines the interface for a user extracting data from a prt stream. Subclasses of this will
 * implement the specific source of the prt data. Ex. prt_ifstream reads from a file.
 */
class prt_istream{
	/**
	 * This internal class is used for storing information about how to extract a channel from a PRT file's particle.
	 */
	struct bound_channel{
		void* dest;
		std::size_t arity, src;
		detail::convert_fn_t copyFn;
	};

	//A list of all channels that we want to extract
	std::vector< bound_channel > m_boundChannels;
	
protected:
	//The layout of the particle data from the source (ex. PRT file).
	prt_layout m_layout;

	// The metadata associated with the prt stream.
	std::map< std::string, detail::any > m_metadata;
	
	/**
	 * This abstract function provides the interface for subclasses to produce particle data.
	 * When prt_istream::read_next_particle() is called, it uses read_impl() to get the next
	 * source particle before extracting the channel data to the type and location
	 * specified by the user with bind() calls.
	 * @param dest A pointer to the location where the subclass should read a single particle
	 *             with layout 'm_layout'.
	 * @note dest must point to an array of at least m_layout.size() bytes.
	 * @return True if a particle was read, or false otherwise. A return of false indicates EOF.
	 */
	virtual bool read_impl( char* dest ) = 0;

public:
	prt_istream()
	{}

	virtual ~prt_istream()
	{}

	/**
	 * Retrieve the channel layout information
	 * @return A constant reference to prt_layout
	 */
	const prt_layout& layout() const {
		return m_layout;
	}

	/**
	 * Determines if the stream's particles have a channel with the given name
	 * @param name The name of the channel to look for.
	 * @return True if a channel with the given name is found, false otherwise.
	 */
	bool has_channel( const std::string& name ) const {
		return m_layout.has_channel( name );
	}
	
#ifdef _WIN32
	/**
	 * Acquires a pointer to a metadata item with a string value.
	 * \param name The name of the metadata item to search for.
	 * \return A pointer to a UTF-16 encoded string value for the named metadata, or NULL if there was no such item.
	 */
	const std::wstring* get_metadata_string( const std::string& name ) const {
		std::map< std::string, detail::any >::const_iterator it = m_metadata.find( name );
		if( it != m_metadata.end() )
			return it->second.get_ptr<std::wstring>();
		return NULL;
	}
#else
	/**
	 * Acquires a pointer to a metadata item with a string value.
	 * \param name The name of the metadata item to search for.
	 * \return A pointer to a UTF-8 encoded string value for the named metadata, or NULL if there was no such item.
	 */
	const std::string* get_metadata_string( const std::string& name ) const {
		std::map< std::string, detail::any >::const_iterator it = m_metadata.find( name );
		if( it != m_metadata.end() )
			return it->second.get_ptr<std::string>();
		return NULL;
	}
#endif
	
	/**
	 * Acquires a pointer to a metadata item with a numeric value.
	 * \tparam T The numeric type of the stored value.
	 * \param name The name of the metadata item to search for.
	 * \return A pointer to a std::vector<T> of the numeric values stored for the metadata item, or NULL if no such named metadata exists.
	 */
	template <class T>
	const std::vector<T>* get_metadata_ptr( const std::string& name ) const {
		std::map< std::string, detail::any >::const_iterator it = m_metadata.find( name );
		if( it != m_metadata.end() )
			return it->second.get_ptr< std::vector<T> >();
		return NULL;
	}

	/**
	 * This template function will bind a user-supplied variable to a named channel extracted from a prt_istream
	 * @tparam T The type of the variable to bind to.
	 * @param name The name of the channel in the prt_istream to bind to.
	 * @param dest A pointer to the destination for the extracted data.
	 * @param arity The size of the array pointed to by 'dest'.
	 */
	template <typename T>
	void bind( const std::string& name, T dest[], std::size_t arity ){
		const detail::prt_channel& ch = m_layout.get_channel( name );

		if( !detail::is_compatible( data_types::traits<T>::data_type(), ch.type ) ){
			std::stringstream ss;
			ss << "Incompatible types for channel \"" << name << "\"";
			ss << ", cannot convert from type: \"" << data_types::names[ ch.type ] << "\"";
			ss << "to: \"" << data_types::names[ data_types::traits<T>::data_type() ] << "\"";

			throw std::runtime_error( ss.str() );
		}

		if( arity != ch.arity ){
			std::stringstream ss;
			ss << "Incompatible types for channel \"" << name << "\"";
			ss << ", cannot convert from arity: \"" << ch.arity << "\"";
			ss << "to: \"" << arity << "\"";

			throw std::runtime_error( ss.str() );
		}

		bound_channel result;
		result.dest = dest;
		result.src = ch.offset;
		result.arity = ch.arity;
		result.copyFn = detail::get_read_converter<T>( ch.type );

		if( !result.copyFn )
			throw std::logic_error( "The channel \"" + name + "\" had an unsupported type: \"" + data_types::names[ ch.type ] + "\"" );

		m_boundChannels.push_back( result );
	}

	/**
	 * This reads the next particle, and extracts the channels requested via bind(). Returns false
	 * if no particle was read (due to EOF).
	 * @return True if a particle was extracted, false if EOF prevented a particle from being read.
	 */
	bool read_next_particle(){
		//Allocate some temporary stack space for the source particle.
		char* data = (char*)alloca( m_layout.size() );

		bool result = this->read_impl( data );
		if( result ){
			//If we read a particle from the source, extract the channel data as requested by the user.
			for( std::vector< bound_channel >::iterator it = m_boundChannels.begin(), itEnd = m_boundChannels.end(); it != itEnd; ++it )
				it->copyFn( it->dest, data + it->src, it->arity );
		}

		return result;
	}
};

}//namespace prtio
