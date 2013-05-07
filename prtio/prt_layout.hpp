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
 * This file contains the classes for working with prt data's memory layout.
 */

#pragma once

#include <prtio/detail/data_types.hpp>
#include <prtio/prt_transforms.hpp>

#include <exception>
#include <locale>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>

namespace prtio{

class prt_istream;
class prt_ostream;

namespace detail{
	//Stores the a PRT channel's offset from the start of the particle, as well as its type and arity.
	struct prt_channel{
		std::size_t offset, arity;
		data_types::enum_t type;
		channel_transformation::option xformType;
	};
	
	//Valid names match the regular exrpession [a-zA-Z_][0-9a-zA-Z_]*
	bool is_valid_name( const std::string& name ){
		// We only allow the standard C locale for characters in names. This means single byte characters with value less than 127.
		std::locale loc( "C" );
		
		std::string::const_iterator it = name.begin(), itEnd = name.end();
		if( it == itEnd )
			return false;
		
		if( !std::isalpha(*it, loc) && *it != '_' )
			return false;
		
		++it;
		
		for( ; it != itEnd; ++it ){
			if( !std::isalnum(*it, loc) && *it != '_' )
				return false;
		}
		
		return true;
	}
}//namespace detail

/**
 * This class represents the layout of a particle in the PRT file. It is created and used by the prt_istream and
 * subclasses.
 */
class prt_layout{
	std::map<std::string, detail::prt_channel> m_channelMap;
	std::vector<std::string> m_channels; //Stores the name of each channel, for easy integer indexing.
	std::size_t m_totalSize;

private:
	friend class prt_istream;
	friend class prt_ostream;

	//private constructor so it can only be created by friend classes.
	prt_layout() : m_totalSize( 0 )
	{}

public:
	/**
	 * Adds a named channel if it does not already exist.
	 * @param name The name of the channel to add to the layout
	 * @param type The data type of the channel
	 * @param arity The number of grouped elements used by this channel. A 3D vector [x,y,z] has arity 3.
	 * @param offset The channel;'s offset in bytes from the beginning of the particle.
	 * @param xformType The transformation to apply to this channel when transforming the data in 3D space (ie. How scale, rotation, translation, etc. affect this data).
	 */
	void add_channel( const std::string& name, data_types::enum_t type, std::size_t arity, std::size_t offset, channel_transformation::option xformType = channel_transformation::unspecified ){
		if( !detail::is_valid_name( name ) )
			throw std::runtime_error( "Invalid channel name \"" + name + "\"" );
		
		std::map<std::string, detail::prt_channel>::iterator it = m_channelMap.find( name );
		if( it != m_channelMap.end() )
			throw std::runtime_error( "Duplicate channel \"" + name + "\" detected" );
		
		if( xformType == channel_transformation::unspecified ){
			if( name == "Position" )
				xformType = channel_transformation::point;
			//... TODO: Add more defaults.
		}
		
		if( !channel_transformation::is_compatible(xformType, type, arity) )
			throw std::runtime_error( "Incompatible transformation for channel \"" + name + "\" detected" );
			
		detail::prt_channel& dest = m_channelMap[ name ];

		dest.type = type;
		dest.arity = arity;
		dest.offset = offset;
		dest.xformType = xformType;

		m_channels.push_back( name );

		m_totalSize += data_types::sizes[ type ] * arity;
	}

	/**
	 * Clears all channels from the layout.
	 */
	void clear(){
		m_channelMap.clear();
		m_totalSize = 0;
	}

	/**
	 * Gets the number of channels in this layout
	 */
	std::size_t num_channels() const {
		return m_channelMap.size();
	}

	/**
	 * Returns true if the layout has a channel with the given name.
	 * @param name The name of the channel to look for.
	 * @return True if the channel exists in this layout, false otherwise.
	 */
	bool has_channel( const std::string& name ) const {
		return m_channelMap.find( name ) != m_channelMap.end();
	}

	/**
	 * Gets the name of the i'th channel in the layout
	 */
	const std::string& get_channel_name( std::size_t index ) const {
		return m_channels[index];
	}

	/**
	 * Returns a const reference to a channel if it exists, otherwise throws a std::out_of_range exception.
	 * @param name The name of the channel to retrieve
	 * @return A const reference to the channel with the given name.
	 */
	const detail::prt_channel& get_channel( const std::string& name ) const {
		std::map<std::string, detail::prt_channel>::const_iterator it = m_channelMap.find( name );
		if( it != m_channelMap.end() )
			return it->second;
		throw std::out_of_range( "There is no channel named \"" + name + "\"" );
	}

	/**
	 * @return The size of a particle with the current layout.
	 */
	std::size_t size() const {
		return m_totalSize;
	}
};

}//namespace prtio