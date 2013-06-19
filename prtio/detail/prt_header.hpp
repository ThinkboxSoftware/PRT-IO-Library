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
 * This file contains support functions for working with the header of a PRT file.
 */

#pragma once

#include <prtio/detail/data_types.hpp>

namespace prtio{

//This namespace contains implementation details for reading PRT files
namespace detail{
	typedef prtio::data_types::int32_t prt_int32;
	typedef prtio::data_types::int64_t prt_int64;

	//This is the layout of a PRT file's main header
	struct prt_header_v1 {
		prt_int64 magicNumber;
		prt_int32 headerLength;
		char      fmtIdentStr[32];
		prt_int32 version;
		prt_int64 particleCount;
	};

	//This is the layout of a PRT file's per-channel header
	struct prt_channel_header_v1 {
		char      channelName[32];
		prt_int32 channelType;
		prt_int32 channelArity;
		prt_int32 channelOffset;
	};

	//Returns the 8 byte magic number that indicates this file format
	inline prt_int64 prt_magic_number(){
		static const unsigned char magic[] = {192, 'P', 'R', 'T', '\r', '\n', 26, '\n'};
		return *(prt_int64*)magic;
	}

	//Returns the human readable signature string to embed in the file
	inline const char* prt_signature_string(){
		return "Extensible Particle Format";
	}
	
	inline prt_int32 prt_meta_chunk(){
		static const char magic[] = {'M', 'e', 't', 'a'};
		return *reinterpret_cast<const prt_int32*>( magic );
	}
	
	inline prt_int32 prt_stop_chunk(){
		static const char magic[] = {'S', 't', 'o', 'p'};
		return *reinterpret_cast<const prt_int32*>( magic );
	}
	
	inline bool is_valid_channel_name( const char* name ){
		if( !std::isalpha(*name) && *name != '_' )
			return false;
		
		std::size_t length = 1;
		
		while( *++name != '\0' ){
			if( ++length >= 32 || (!std::isalnum(*name) && *name == '_') )
				return false;
		}
		
		return true;
	}
}//namespace detail

}//namespace prtio
