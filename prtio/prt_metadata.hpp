#pragma once

#include <prtio/prt_istream.hpp>
#include <prtio/prt_ostream.hpp>

namespace prtio{

namespace distance_unit{
	enum option{
		unitless,
		inches,
		feet,
		miles,
		millimeters,
		centimeters,
		meters,
		kilometers,
		invalid
	};
	
	inline const char* to_string( option unitType ){
		static const char* names[] = { "unitless", "inches", "feet", "miles", "millimeters", "centimeters", "meters", "kilometers", "invalid" };
		
		return names[unitType];
	}
};

namespace coordinate_system{
	enum option{
		unspecified,
		left_handed_xup,
		left_handed_yup,
		left_handed_zup,
		right_handed_xup,
		right_handed_yup,
		right_handed_zup,
		invalid
	};
	
	enum{
		max = right_handed_zup,
		maya = right_handed_yup,
		xsi = right_handed_yup,
		houdini = right_handed_yup,
		cinema4d = left_handed_yup,
		realflow = left_handed_yup,
	};
	
	inline const char* to_string( option sysType ){
		static const char* names[] = { "unspecified", "left_handed_xup", "left_handed_yup", "left_handed_zup", "right_handed_xup", "right_handed_yup", "right_handed_zup", "invalid" };
		
		return names[sysType];
	}
};

/** 
 * Stores the unit type that distance/length based channels like Position, Velocity, etc. are measured in.
 * \param stream The prt_ostream to set the metadata for.
 * \param distanceUnit The unit the particle data is stored as.
 */
inline void set_distance_unit( prt_ostream& stream, distance_unit::option distanceUnit ){
	data_types::int32_t distanceUnitInt = static_cast<data_types::int32_t>( distanceUnit );
	
	stream.add_metadata( "DistanceUnit", &distanceUnitInt, 1u );
}

/**
 * Reads the unit that distance/length based channels (ex. Position) are stored with. This allows for scaling the particles
 * to appropriately fit the context in which they are loaded.
 * \param stream The stream to query
 * \return The units used in the queried stream.
 */
inline distance_unit::option get_distance_unit( prt_istream& stream ){
	if( const std::vector<data_types::int32_t>* pValue = stream.get_metadata_ptr<data_types::int32_t>( "DistanceUnit" ) ){
		if( pValue->size() != 1u || pValue->front() < distance_unit::unitless || pValue->front() >= distance_unit::invalid )
			return distance_unit::invalid;
		return static_cast<distance_unit::option>( pValue->front() );
	}
	
	return distance_unit::unitless;
}

/**
 * Specifies the up-vector and handedness of the coordinate system used for generating particle data.
 * \param stream The stream to assign the coordinate system to.
 * \param coordSysType The type of coordinate system. In 3ds Max this would be 'coordinate_system::right_handed_zup'.
 */
inline void set_coordinate_system( prt_ostream& stream, coordinate_system::option coordSysType ){
	data_types::int32_t coordSysInt	= static_cast<data_types::int32_t>( coordSysType );
	
	stream.add_metadata( "CoordSys", &coordSysInt, 1u );
}

/**
 * Reads the type of coordinate system used by the contained particle data. This indicates the up-vector and handedness of the
 * coordinate system.
 * \param stream The stream to query
 * \return The type of coordinate system used by the queried stream
 */
inline coordinate_system::option get_coordinate_system( prt_istream& stream ){
	if( const std::vector<data_types::int32_t>* pValue = stream.get_metadata_ptr<data_types::int32_t>( "CoordSys" ) ){
		if( pValue->size() != 1u || pValue->front() < coordinate_system::unspecified || pValue->front() >= coordinate_system::invalid )
			return coordinate_system::invalid;
		return static_cast<coordinate_system::option>( pValue->front() );
	}
	
	return coordinate_system::unspecified;
}

/**
 * Specifies the framerate (in frames per second) of the program which is generating particle data. The framerate is stored as a fraction composed
 * of a numerator and denominator pair. Ex. 30/1 is 30FPS, while NTSC television is 24000/1001 = 23.976FPS.
 * \param stream The stream to set the framerate for.
 * \param fpsNumerator The numerator of the framerate fraction.
 * \param fpsDenominator The denominator of the framerate fraction.
 */
inline void set_framerate( prt_ostream& stream, unsigned fpsNumerator, unsigned fpsDenominator ){
	data_types::uint32_t fps[] = { fpsNumerator, fpsDenominator };
	
	stream.add_metadata( "FrameRate", fps, 2u );
}

/**
 * Reads the framerate (as a fraction indicating frames per second) associated with a particle stream.
 * \param stream The stream to query
 * \param[out] outFps The numerator/denominator pair that recieves the framrate.
 * \return True if 'outFps' was filled correctly with the stream's framerate. False indicates the framerate was not stored.
 */
inline bool get_framerate( prt_istream& stream, unsigned (&outFps)[2] ){
	if( const std::vector<data_types::uint32_t>* pValues = stream.get_metadata_ptr<data_types::uint32_t>( "FrameRate" ) ){
		if( pValues->size() == 2u ){
			std::copy( pValues->begin(), pValues->end(), outFps );
			return true;
		}
	}
	return false;
}

/**
 * Reads the bounding box of the particles contained in a stream.
 * \param stream The particle stream to query
 * \param[out] outMin Receives the lower bound of the box (as an XYZ coordinate).
 * \param[out] outMax Receives the upper bound of the box (as an XYZ coordinate).
 * \return True if the bounding box was stored with the stream.
 */
inline bool get_boundbox( prt_istream& stream, float (&outMin)[3], float (&outMax)[3] ){
	if( const std::vector<data_types::float32_t>* pValues = stream.get_metadata_ptr<data_types::float32_t>( "BoundBox" ) ){
		if( pValues->size() == 6u ){
			std::copy( pValues->begin(), pValues->begin() + 3, outMin );
			std::copy( pValues->begin() + 3, pValues->end(), outMax );
			return true;
		}
	}
	return false;
}

}
