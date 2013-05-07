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
 * This file contains functions for working with transformation metadata for channels.
 */
#pragma once

#include <prtio/detail/data_types.hpp>
#include <prtio/detail/conversion.hpp>

namespace prtio{

namespace channel_transformation{
	/**
	 * These options are all provided in the context of a 4x4 homogenous matrix used to apply a transformation in 3D space. 
	 */
	enum option{
		unspecified,    // Not affected by transformations (ex. RGB Color, integer ID, etc.)
		point,          // Normal 3D transformation                                         -> Apply the transformation directly
		vector,         // Not affected by translation                                      -> Use the matrix without translation to modify the channel
		normal,         // Not affected by translation or skew (maybe not scale either...)  -> Use the transpose of the inverted matrix (without translation) to modify the channel
		orientation,    // Only affected by rotation                                        -> Extract the rotation part of the transformation (maybe via polar decomposition) and apply it as a quaternion.
		rotation,       // Only affected by rotation                                        -> Extract the rotation part of the transformation (maybe via polar decomposition) and apply it to the axis part of this channel.
		scalar,         // Only affected by scale                                           -> Extract the scale part of the transformation (maybe via polar decomposition) and apply the largest eigenvalue as a scalar.
		invalid
	};
	
	inline const char* to_string( option val ){
		static const char* names[] = { "unspecified", "point", "vector", "normal", "orientation", "rotation", "scalar", "invalid" };
		
		return names[val];
	}
	
	bool is_compatible(option xformType, data_types::enum_t type, std::size_t arity){
		switch( xformType ){
		case point:
		case vector:
		case normal:
			return detail::is_float(type) && arity == 3u; 
		case orientation:
		case rotation:
			return detail::is_float(type) && arity == 4u;
		case scalar:
			return detail::is_float(type) && arity == 1u;
		default:
			return true;
		}
	}
}

}
