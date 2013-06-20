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
 * This file contains the definition of a stream for writing prt files. For more information on the
 * PRT file specification, refer to http://www.thinkboxsoftware.com/krak-prt-file-format/
 */

#pragma once

#include <prtio/prt_ostream.hpp>
#include <prtio/detail/prt_header.hpp>
#include <cassert>
#include <fstream>
#include <limits>
#include <zlib.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace prtio{

/**
 * This class implements the prt_istream interface, for reading particles from a file.
 */
class prt_ofstream : public prt_ostream{
	std::string m_filePath; //The path to the PRT file.
	std::ofstream m_fout;   //The stream that is writing bytes to the file.
	z_stream m_zstream;     //The zlib stream that is compressing particles for writing to the file.

	char* m_buffer;           //A temporary buffer for storing the compressed file data before being flushed to disk.
	std::size_t m_bufferSize; //The size of 'm_buffer' in bytes.

	detail::prt_int64 m_particleCount; //The number of particles written so far.

	std::ostream::streampos m_countLocation; //The location that we need to write the final particle count to.
	std::ostream::streampos m_boundBoxLocation; //The location that we need to write the final boundbox to.
	
	float m_bounds[6];
	std::ptrdiff_t m_posChannelOffset;
	
private:
	static std::size_t get_value_size( const prt_meta_value& value ){
		if( value.get_type() == meta_types::type_string ){
#ifdef _WIN32
			// Convert to UTF8 before measuring
			int result = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, value.get_string(), -1, NULL, 0, NULL, NULL );
			if( result <= 0 ) // If there was an error, return 1 so we can make an empty string.
				return 1u;
			return static_cast<std::size_t>( result );
#else
			return std::char_traits<char>::length( value.get_string() ) + 1;
#endif
		}else{
			return value.get_arity() * data_types::sizes[ value.get_type() ];
		}
	}
	
	void write_value( const prt_meta_value& value ){
		if( value.get_type() == meta_types::type_string ){
			const uchar_type* pString = value.get_string();
		
#ifdef _WIN32
			std::vector<char> convertedString;

			// Convert to UTF8 before measuring
			int result = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, pString, -1, NULL, 0, NULL, NULL );
			if( result <= 0 ){
				convertedString.push_back( '\0' );
			}else{
				convertedString.assign( static_cast<std::size_t>( result ), '\0' );
				
				result = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, pString, -1, &convertedString.front(), result, NULL, NULL );
			}
			
			m_fout.write( &convertedString.front(), convertedString.size() );
#else
			m_fout.write( pString, std::char_traits<char>::length( pString ) + 1 );
#endif
		}else{
			m_fout.write( static_cast<const char*>( value.get_void_ptr() ), value.get_arity() * data_types::sizes[ value.get_type() ] );
		}
	}
	
	static std::size_t measure_meta_chunk( const std::string& channelName, const std::string& valueName, const prt_meta_value& value ){
		return channelName.size() + 1 + valueName.size() + 1 + 4 + get_value_size( value );
	}
	
	/**
	 * Writes a meta chunk to the stream.
	 * @return The stream location of the value in case it needs to be re-written later.
	 */
	std::ostream::streampos write_meta_chunk( const std::string& channelName, const std::string& valueName, const prt_meta_value& value ){
		if( !detail::is_valid_channel_name( valueName.c_str() ) )
			throw std::runtime_error( "Invalid metadata name: \"" + valueName + "\" for channel: \"" + channelName + "\" while writing file: \"" + m_filePath + "\"" );
		
		detail::prt_int32 chunkType = detail::prt_meta_chunk();
		detail::prt_int32 chunkLength = measure_meta_chunk( channelName, valueName, value );
		
		m_fout.write( reinterpret_cast<const char*>( &chunkType ), 4 );
		m_fout.write( reinterpret_cast<const char*>( &chunkLength ), 4 );
		
		detail::prt_int32 valueType = value.get_type();

		m_fout.write( channelName.c_str(), channelName.size() + 1 );
		m_fout.write( valueName.c_str(), valueName.size() + 1 );
		m_fout.write( reinterpret_cast<const char*>( &valueType ), 4 );
		
		std::ostream::streampos result = m_fout.tellp();
		
		this->write_value( value );
		
		return result;
	}
	
	static std::size_t measure_stop_chunk(){
		return 0;
	}
	
	void write_stop_chunk(){
		detail::prt_int32 chunkType = detail::prt_stop_chunk();
		detail::prt_int32 chunkLength = 0;
		
		m_fout.write( reinterpret_cast<const char*>( &chunkType ), 4 );
		m_fout.write( reinterpret_cast<const char*>( &chunkLength ), 4 );
	}
	
	/**
	 * This function writes the uncompressed PRT file header, and records the file pointer position in order to later write the number of particles
	 * for the file. It expects 'm_layout' to not change afterwards, or else you are a bad human/android/robot.
	 */
	void write_header(){
		using namespace detail;

		// Force the BoundBox metadata to exist, and also set it to empty since we haven't calculated the box yet.
		m_fileMetadata[ "BoundBox" ].set_array( m_bounds );
		
		// Calculate the length of the various chunks in the header;
		std::size_t chunksSizeTotal = 0;
		
		for( std::map< std::string, prt_meta_value >::const_iterator it = m_fileMetadata.begin(), itEnd = m_fileMetadata.end(); it != itEnd; ++it )
			chunksSizeTotal += 8 + measure_meta_chunk( "", it->first, it->second );
		
		for( std::map< std::string, std::map< std::string, prt_meta_value > >::const_iterator it = m_channelMetadata.begin(), itEnd = m_channelMetadata.end(); it != itEnd; ++it ){
			if( !m_layout.has_channel( it->first ) )
				continue;
				
			for( std::map< std::string, prt_meta_value >::const_iterator itValue = it->second.begin(), itValueEnd = it->second.end(); itValue != itValueEnd; ++itValue )
				chunksSizeTotal += 8 + measure_meta_chunk( it->first, itValue->first, itValue->second );
		}
		
		chunksSizeTotal += 8 + measure_stop_chunk();
		
		// Write the main header data
		prt_header_v1 header;
		memset( &header, 0, sizeof(prt_header_v1) );

		header.magicNumber = prt_magic_number();
		header.headerLength = sizeof(prt_header_v1) + chunksSizeTotal;
		strncpy(header.fmtIdentStr, prt_signature_string(), 32);
		header.version = 2;
		header.particleCount = -1;

		m_countLocation = ((char*)&header.particleCount - (char*)&header) + m_fout.tellp(); //This is where we need to seek to in order to write the particle count at the end.

		//m_fout.write(reinterpret_cast<const char*>(&header), sizeof(prt_header_v1));
		m_fout.write( reinterpret_cast<const char*>( &header.magicNumber ), 8 );
		m_fout.write( reinterpret_cast<const char*>( &header.headerLength ), 4 );
		m_fout.write( header.fmtIdentStr, 32 );
		m_fout.write( reinterpret_cast<const char*>( &header.version ), 4 );
		m_fout.write( reinterpret_cast<const char*>( &header.particleCount ), 8 );
		
		for( std::map< std::string, prt_meta_value >::const_iterator it = m_fileMetadata.begin(), itEnd = m_fileMetadata.end(); it != itEnd; ++it ){
			std::ostream::streampos p = this->write_meta_chunk( "", it->first, it->second );
			
			if( it->first == "BoundBox" )
				m_boundBoxLocation = p;
		}
		
		for( std::map< std::string, std::map< std::string, prt_meta_value > >::const_iterator it = m_channelMetadata.begin(), itEnd = m_channelMetadata.end(); it != itEnd; ++it ){
			if( !m_layout.has_channel( it->first ) )
				continue;
			
			for( std::map< std::string, prt_meta_value >::const_iterator itValue = it->second.begin(), itValueEnd = it->second.end(); itValue != itValueEnd; ++itValue )
				this->write_meta_chunk( it->first, itValue->first, itValue->second );
		}
		
		this->write_stop_chunk();
		
		assert( m_fout.tellp() == static_cast<std::streamsize>(header.headerLength) );

		// Write the reserved bytes
		prt_int32 reservedInt = 4;
		m_fout.write(reinterpret_cast<const char*>(&reservedInt), 4);

		// Write the channel map
		prt_channel_header_v1 prtChannel;

		prt_int32 channelCount = static_cast<prt_int32>( m_layout.num_channels() );
		prt_int32 channelHeaderItemSize = sizeof(prt_channel_header_v1);

		m_fout.write(reinterpret_cast<const char*>(&channelCount), 4);
		m_fout.write(reinterpret_cast<const char*>(&channelHeaderItemSize), 4);

		for( int i = 0; i < channelCount; ++i ) {
			memset( &prtChannel, 0, sizeof(prt_channel_header_v1) );

			std::string chName = m_layout.get_channel_name( static_cast<std::size_t>(i) );
			
			if( !detail::is_valid_channel_name( chName.c_str() ) )
				throw std::runtime_error( "Invalid channel name: \"" + chName + "\" while writing file: \"" + m_filePath + "\"" );
			
			const detail::prt_channel& ch = m_layout.get_channel( chName );

			strncpy( prtChannel.channelName, chName.c_str(), 32 );
			prtChannel.channelArity = (prt_int32)ch.arity;
			prtChannel.channelType = (prt_int32)ch.type;
			prtChannel.channelOffset = (prt_int32)ch.offset;

			m_fout.write(reinterpret_cast<const char*>(&prtChannel), sizeof(prt_channel_header_v1));
		}
	}

	/**
	 * This function initializes the zlib decompression stream for the particle data portion of the PRT file.
	 */
	void init_zlib(){
		if(Z_OK != deflateInit( &m_zstream, Z_DEFAULT_COMPRESSION ) )
			throw std::runtime_error( "Unable to initialize a zlib deflate stream for output stream \"" + m_filePath + "\"." );

		if( m_bufferSize == 0 )
			m_bufferSize = (1 << 19);

		m_buffer = new char[m_bufferSize];

		m_zstream.avail_out = static_cast<unsigned int>( m_bufferSize );
		m_zstream.next_out = reinterpret_cast<unsigned char*>( m_buffer );
	}

	/**
	 * This helper function will write the compressed data stored in 'm_buffer' to disk.
	 */
	void flush(){
		std::size_t numOut = (m_bufferSize - m_zstream.avail_out);
		if( numOut > 0 ) {
			m_fout.write( m_buffer, numOut );
			m_zstream.avail_out = static_cast<unsigned int>( m_bufferSize );
			m_zstream.next_out = reinterpret_cast<unsigned char*>( m_buffer );
		}
	}

public:
	/**
	 * Default constructor. User must later call open().
	 */
	prt_ofstream(){
		m_buffer = NULL;
		m_bufferSize = 0;
		m_particleCount = 0;
		m_countLocation = 0;
		m_boundBoxLocation = 0;
		m_posChannelOffset = -1;
		memset( &m_zstream, 0, sizeof(m_zstream) );
		
		m_bounds[0] = m_bounds[1] = m_bounds[2] = (std::numeric_limits<float>::max)();
		m_bounds[3] = m_bounds[4] = m_bounds[5] = (std::numeric_limits<float>::min)();
	}

	/**
	 * Can't have a constructor that opens the stream too, since we need to know the layout first!
	 */
	 //prt_ofstream( const std::string& file );

	virtual ~prt_ofstream(){
		close();
	}

	/**
	 * Opens the prt_ofstream to write to the specified file
	 * @param file Path to the file to write particles to
	 */
	void open( const std::string& file ){
		m_fout.open( file.c_str(), std::ios::out | std::ios::binary );
		if( m_fout.fail() )
			throw std::ios_base::failure( "Failed to open file \"" + file + "\" for writing" );

		m_filePath = file;
		m_fout.exceptions( std::ios::badbit|std::ios::failbit ); //We want an exception if writing anything fails.
		
		if( m_layout.has_channel( "Position" ) ){
			const detail::prt_channel& ch = m_layout.get_channel( "Position" );
			if( ch.type == data_types::type_float32 && ch.arity == 3 )
				m_posChannelOffset = static_cast<std::ptrdiff_t>( ch.offset );
		}
		
		write_header();
		init_zlib();
	}

	/**
	 * Closes the stream, and deallocates any memory used for decompressing particles.
	 */
	void close(){
		if( m_buffer ){
			// Write out all the rest of the stream data, until we hit Z_STREAM_END
			while(Z_STREAM_END != deflate(&m_zstream, Z_FINISH))
				flush();
			flush();

			delete[] m_buffer;
			m_buffer = NULL;

			int ret = deflateEnd(&m_zstream);
			if( ret != Z_OK )
				throw std::runtime_error("deflateEnd() error for \"" + m_filePath + "\" was:" + zError(ret) );

			memset( &m_zstream, 0, sizeof(z_stream) );
		}

		//Seek back to the beginning of the file and write the particle count in the header region.
		if( m_fout.is_open() ){
			if( m_countLocation > 0 ){
				m_fout.seekp( m_countLocation, std::ios::beg );
				m_fout.write( reinterpret_cast<const char*>( &m_particleCount ), 8 );
			}
			if( m_boundBoxLocation > 0 ){
				m_fout.seekp( m_boundBoxLocation, std::ios::beg );
				m_fout.write( reinterpret_cast<const char*>( m_bounds ), sizeof(float) * 6 );
			}
			m_fout.close();
		}

		m_filePath.clear();
		m_layout.clear();

		m_bufferSize = 0;
		m_particleCount = 0;
		m_countLocation = 0;
	}

protected:
	/**
	 * Compresses a single particle into 'm_buffer' and flushes to disk if the buffer is full.
	 * @param data The data for the particle to write to disk.
	 */
	virtual void write_impl( const char* data ){
		++m_particleCount;
		
		// If we have a valid Position channel, add this particle to bounding box we are tracking.
		if( m_posChannelOffset >= 0 ){
			const float* p = reinterpret_cast<const float*>( data + m_posChannelOffset );
			if( p[0] < m_bounds[0] )
				m_bounds[0] = p[0];
			if( p[0] > m_bounds[3] )
				m_bounds[3] = p[0];
				
			if( p[1] < m_bounds[1] )
				m_bounds[1] = p[1];
			if( p[1] > m_bounds[4] )
				m_bounds[4] = p[1];
				
			if( p[2] < m_bounds[2] )
				m_bounds[2] = p[2];
			if( p[2] > m_bounds[5] )
				m_bounds[5] = p[2];
		}
		
		m_zstream.avail_in = static_cast<unsigned>( m_layout.size() );
		m_zstream.next_in = reinterpret_cast<unsigned char*>( const_cast<char*>(data) );

		for(;;) {
			int ret = deflate(&m_zstream, Z_NO_FLUSH);
			if(ret == Z_STREAM_ERROR)
				throw std::runtime_error( "deflate() call writing to \"" + m_filePath + "\" failed:\n\t" + zError(ret) );

			if(m_zstream.avail_out == 0) {	//Did we fill the output buffer completely? If so flush and loop.
				flush();
			} else {
				break;
			}
		}
	}
};

}//namespace prtio
