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
 * This file contains the definition of a stream for reading prt files. For more information on the
 * PRT file specification, refer to http://www.thinkboxsoftware.com/krak-prt-file-format/
 */

#pragma once

#include <prtio/prt_istream.hpp>
#include <prtio/detail/prt_header.hpp>
#include <fstream>
#include <zlib.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace prtio{

/**
 * This class implements the prt_istream interface, for reading particles from a file.
 */
class prt_ifstream : public prt_istream{
	std::string m_filePath; //The path to the PRT file.
	std::ifstream m_fin;    //The stream that is reading bytes from the file.
	z_stream m_zstream;     //The zlib stream that is decompressing particle from the file.

	char* m_buffer;           //A temporary buffer for storing the compressed file data before being unzipped.
	std::size_t m_bufferSize; //The size of 'm_buffer' in bytes.

	detail::prt_int64 m_particleCount; //The number of particles remaining in the file.

private:
	void read_meta_chunk( detail::prt_int32 chunkLength ){
		char channelName[32];
		char valueName[32];
		detail::prt_int32 valueType;
		
		m_fin.getline( channelName, 32, '\0' );
		chunkLength -= m_fin.gcount();
		
		m_fin.getline( valueName, 32, '\0' );
		chunkLength -= m_fin.gcount();
		
		m_fin.read( reinterpret_cast<char*>( &valueType ), 4u );
		chunkLength -= 4;
		
		assert( chunkLength > 0 );
		
		if( valueType < meta_types::type_string || valueType >= meta_types::type_last )
			throw std::runtime_error( std::string() + "The data type specified for channel \"" + channelName + "\" metadata \"" + valueName + "\" in the input stream \"" + m_filePath + "\" is not valid." );
		
		struct scoped_void_ptr{
			void* ptr;
			~scoped_void_ptr(){
				operator delete( ptr );
			}
		} buffer = { operator new( chunkLength ) };
		
		// We've been subtracting the read bytes as we go, so this is the remaining number of bytes in the chunk.
		m_fin.read( static_cast<char*>( buffer.ptr ), chunkLength );
		
		if( channelName[0] != '\0' && !detail::is_valid_channel_name( channelName ) ){
			std::cerr << "Invalid channel name: \"" << channelName << "\" in metadata: \"" << valueName << "\"" << std::endl;
			return;
		}
		
		if( !detail::is_valid_channel_name( valueName ) ){
			std::cerr << "Invalid metadata name: \"" << valueName << "\" in channel: \"" << channelName << "\"" << std::endl;
			return;
		}
		
		std::map< std::string, prt_meta_value >& metaValue = (channelName[0] == '\0') ? m_fileMetadata : m_channelMetadata[ std::string(channelName) ];
		
		if( valueType == meta_types::type_string ){
#ifdef _WIN32
			int wcharLength = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, static_cast<const char*>( buffer.ptr ), chunkLength, NULL, 0 );
			if( wcharLength <= 0 ){
				std::cerr << "Invalid string data in metadata: \"" << valueName << "\" in channel: \"" << channelName << "\"" << std::endl;
				return;
			}

			// In Windows we convert from UTF8 to UTF16LE in memory.
			std::vector<wchar_t> convertedString( wcharLength, L'\0' );
			
			int r = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, static_cast<const char*>( buffer.ptr ), chunkLength, &convertedString.front(), wcharLength );
			
			assert( r > 0 );
			
			metaValue[ std::string(valueName) ].set_string( &convertedString.front(), static_cast<std::size_t>( chunkLength - 1 ) );
#else
			// In non-Windows we can work directly with UTF8.
			metaValue[ std::string(valueName) ].set_string( static_cast<char*>( buffer.ptr ), static_cast<std::size_t> ( chunkLength - 1 ) );
#endif
		}else{
			std::size_t basicSize = data_types::sizes[valueType]; // We've already verified that we have a valid index.
			
			assert( chunkLength >= basicSize && (chunkLength % basicSize) == 0 );
			
			std::size_t arity = chunkLength / basicSize;
			
			metaValue[ std::string(valueName) ].set( static_cast<meta_types::option>( valueType ), arity, buffer.ptr );
		}
	}

	/**
	 * This function reads the uncompressed header portion of the PRT file and leaves the read pointer
	 * of 'm_fin' at the beginning of the compressed particle data portion of the file. It will populate
	 * the member 'm_layout' with the layout of particle data after being decompressed.
	 */
	void read_header(){
		using namespace detail;

		prt_header_v1 header;
		
		//m_fin.read(reinterpret_cast<char*>(&header), sizeof(prt_header_v1));
		m_fin.read( reinterpret_cast<char*>(&header.magicNumber), 8 );
		m_fin.read( reinterpret_cast<char*>(&header.headerLength), 4 );
		m_fin.read( header.fmtIdentStr, 32 );
		m_fin.read( reinterpret_cast<char*>(&header.version), 4 );
		m_fin.read( reinterpret_cast<char*>(&header.particleCount), 8 );

		//This is not a prt file (as opposed to a corrupt prt file);
		if( header.magicNumber != prt_magic_number() )
			throw std::runtime_error( "The input stream \"" + m_filePath + "\" did not contain the .prt file magic number." );

		//This is not a prt file (as opposed to a corrupt prt file);
		if( strncmp(prt_signature_string(), header.fmtIdentStr, 32) != 0 )
			throw std::runtime_error( "The input stream \"" + m_filePath + "\" did not contain the signature string '" + prt_signature_string() + "'." );

		m_particleCount = header.particleCount;
		if( header.particleCount < 0 )
			throw std::runtime_error( "The input stream \"" + m_filePath + "\" was not closed correctly and reported negative particles within." );

		if( header.version <= 1 ){
			// Skip parts of the file header which may have been added since the first version of the .prt format
			if( header.headerLength != sizeof(prt_header_v1) )
				m_fin.seekg(header.headerLength - sizeof(prt_header_v1), std::ios::cur);
		}else{
			detail::prt_int32 chunkType;
			detail::prt_int32 chunkLength;
			
			m_fin.read( reinterpret_cast<char*>( &chunkType ), 4 );
			m_fin.read( reinterpret_cast<char*>( &chunkLength ), 4 );
			
			while( chunkType != detail::prt_stop_chunk() ){
				if( chunkType == detail::prt_meta_chunk() ){
					this->read_meta_chunk( chunkLength );
				}else{
					// Skip this unknown chunk.
					m_fin.seekg( chunkLength, std::ios::cur );
				}

				m_fin.read( reinterpret_cast<char*>( &chunkType ), 4 );
				m_fin.read( reinterpret_cast<char*>( &chunkLength ), 4 );
			}
		}

		prt_int32 attrLength;
		m_fin.read(reinterpret_cast<char*>(&attrLength), 4);

		if( attrLength != 4 )
			throw std::runtime_error( "The reserved int value is not set to 4." );

		prt_int32 channelCount, perChannelLength;
		m_fin.read(reinterpret_cast<char*>(&channelCount), 4);
		m_fin.read(reinterpret_cast<char*>(&perChannelLength), 4);

		for(int i = 0; i < channelCount; ++i){
			prt_channel_header_v1 channel;
			m_fin.read(reinterpret_cast<char*>(&channel), sizeof(prt_channel_header_v1));

			// Make sure the channel name is null terminated
			channel.channelName[31] = '\0';
			
			if( !detail::is_valid_channel_name( channel.channelName ) )
				std::cerr << "Invalid channel name: \"" << channel.channelName << "\"" << std::endl;
			
			if( channel.channelType < 0 || channel.channelType >= data_types::type_count )
				throw std::runtime_error( std::string() + "The data type specified in channel \"" + channel.channelName + "\" in the input stream \"" + m_filePath + "\" is not valid." );

			if( channel.channelArity < 0 )
				throw std::runtime_error( std::string() + "The arity specified in channel \"" + channel.channelName + "\" in the input stream \"" + m_filePath + "\" is not valid." );

			if( channel.channelOffset < 0 )
				throw std::runtime_error( std::string() + "The offset specified in channel \"" + channel.channelName + "\" in the input stream \"" + m_filePath + "\" is not valid." );

			m_layout.add_channel(
				std::string( channel.channelName ),
				static_cast<data_types::enum_t>( channel.channelType ),
				static_cast<std::size_t>( channel.channelArity ),
				static_cast<std::size_t>( channel.channelOffset )
			);
			
			// Force metadata creation for this channel if there weren't any values stored in the file.
			m_channelMetadata[ channel.channelName ];

			if( perChannelLength != sizeof(prt_channel_header_v1) )
				m_fin.seekg(perChannelLength - sizeof(prt_channel_header_v1), std::ios::cur);	//Skip unknown parts of the channel header
		}
		
		// Remove metadata for channels that don't actually exist.
		for( std::map< std::string, std::map< std::string, prt_meta_value > >::iterator it = m_channelMetadata.begin(), itEnd = m_channelMetadata.end(); it != itEnd; /*Do Nothing*/ ){
			if( !m_layout.has_channel( it->first ) ){
				std::cerr << "Invalid metadata channel: \"" << it->first << "\"" << std::endl;
				m_channelMetadata.erase( it++ );
			}else{
				++it;
			}
		}
	}

	/**
	 * This function initializes the zlib decompression stream for the particle data portion of the PRT file.
	 */
	void init_zlib(){
		if(Z_OK != inflateInit(&m_zstream) )
			throw std::runtime_error( "Unable to initialize a zlib inflate stream for input stream \"" + m_filePath + "\"." );

		if( m_bufferSize == 0 )
			m_bufferSize = (1 << 19);

		m_buffer = new char[m_bufferSize];
	}

public:
	/**
	 * Default constructor. User must later call open().
	 */
	prt_ifstream(){
		m_buffer = NULL;
		m_bufferSize = 0;
		m_particleCount = 0;
		memset( &m_zstream, 0, sizeof(m_zstream) );
	}

	/**
	 * Constructor that opens the stream for the given file.
	 * @param filePath Path to the PRT file to read particles from.
	 */
	prt_ifstream( const std::string& filePath ){
		m_buffer = NULL;
		m_bufferSize = 0;
		m_particleCount = 0;
		memset( &m_zstream, 0, sizeof(m_zstream) );

		open( filePath );
	}

	virtual ~prt_ifstream(){
		close();
	}

	/**
	 * Opens the prt_ifstream to read from the specified file
	 * @param file Path to the file to read particles from
	 */
	void open( const std::string& file ){
		m_fin.open( file.c_str(), std::ios::in | std::ios::binary );
		if( m_fin.fail() )
			throw std::ios_base::failure( "Failed to open file \"" + file + "\"" );

		m_filePath = file;
		m_fin.exceptions( std::ios::badbit );

		read_header();
		init_zlib();
	}

	/**
	 * The number of particles remaining in the file.
	 */
	detail::prt_int64 particle_count() const
	{
		return m_particleCount;
	}

	/**
	 * Closes the stream, and deallocates any memory used for decompressing particles.
	 */
	void close(){
		m_filePath.clear();
		m_fin.close();

		if( m_buffer ){
			inflateEnd( &m_zstream );
			memset( &m_zstream, 0, sizeof(z_stream) );

			delete[] m_buffer;
			m_buffer = NULL;
		}

		m_layout.clear();

		m_bufferSize = 0;
		m_particleCount = 0;
	}

protected:
	/**
	 * Reads a single particle from disk into the specified buffer.
	 * @param data The location to read a single particle to. Must be at least m_layout.size() bytes.
	 * @return True if a particle was read, false if EOF or the stream was never opened.
	 */
	virtual bool read_impl( char* data ){
		if( m_particleCount == 0 ){
			if( !m_fin.is_open() || m_fin.eof() )
				return false;
			throw std::runtime_error( "The file \"" + m_filePath + "\" did not contain the number of particles it claimed" );
		}

		m_zstream.avail_out = static_cast<uInt>( m_layout.size() );
		m_zstream.next_out = reinterpret_cast<unsigned char*>(data);

		do{
			if(m_zstream.avail_in == 0){
				m_fin.read(m_buffer, m_bufferSize);

				if( m_fin.fail() && m_bufferSize == 0 )
					throw std::ios_base::failure( "Failed to read from file \"" + m_filePath + "\"" );

				m_zstream.avail_in = static_cast<uInt>(m_fin.gcount());
				m_zstream.next_in = reinterpret_cast<unsigned char*>(m_buffer);
			}

			int ret = inflate(&m_zstream, Z_SYNC_FLUSH);
			if(Z_OK != ret && Z_STREAM_END != ret){
				std::stringstream ss;
				ss << "inflate() on file \"" << m_filePath << "\" ";
				ss << "with " << m_particleCount << " particles left failed:\n\t";
				ss << zError(ret);

				throw std::runtime_error( ss.str() );
			}

		}while(m_zstream.avail_out != 0);

		--m_particleCount;

		return true;
	}
};

}//namespace prtio
