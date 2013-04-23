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
 * This file contains samples of how the library should be used to generate
 * and read data from PRT files.
 */

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <prtio/prt_ifstream.hpp>
#include <prtio/prt_ofstream.hpp>
#include <prtio/prt_metadata.hpp>

#if !defined(__APPLE__) && !defined(_WIN32) && !defined(_WIN64) && __WORDSIZE == 64
#define INT64 long int
#else
#define INT64 long long
#endif

#ifdef _WIN32
typedef std::wstring unicode_string;

#define NOMINMAX
#include <Windows.h>

#define _T( string ) L ## string

std::ostream& operator<<( std::ostream& ostream, const std::wstring& wstr ){
	// Use CP_OEMCP since this is a console app, not a GUI app.
	int r = WideCharToMultiByte( CP_OEMCP, 0, wstr.c_str(), static_cast<int>( wstr.size() ), NULL, 0, NULL, NULL );
	if( r > 0 ){
		std::string str( static_cast<std::size_t>( r ), '\0' );
		
		r = WideCharToMultiByte( CP_OEMCP, 0, wstr.c_str(), static_cast<int>( wstr.size() ), &str[0], r, NULL, NULL );
		if( r > 0 )
			ostream << str;
	}
	
	return ostream;
}
#else
typedef std::string unicode_string;

#define _T( string ) string
#endif


void example_reading( const std::string& filePath ){
	struct{
		float pos[3];
		float vel[3];
		float col[3];
		float density;
		INT64 id;
	} theParticle = { {0,0,0}, {0,0,0}, {1.f,1.f,1.f}, 0.f, -1 };

	prtio::prt_ifstream stream( filePath );

	prtio::coordinate_system::option coordSys = prtio::get_coordinate_system( stream );
	prtio::distance_unit::option distanceUnit = prtio::get_distance_unit( stream );
	
	std::cout << "\tCoordSys = " << prtio::coordinate_system::to_string( coordSys ) << std::endl;
	std::cout << "\tDistanceUnit = " << prtio::distance_unit::to_string( distanceUnit ) << std::endl;
	
	unsigned framerate[] = { 1, 1 };

	if( prtio::get_framerate( stream, framerate ) )
		std::cout << "\tFrameRate = {" << framerate[0] << ", " << framerate[1] << "}" << std::endl;
	
	float boundbox[][3] = { {0.f, 0.f, 0.f}, {0.f, 0.f, 0.f} };
	
	if( prtio::get_boundbox( stream, boundbox[0], boundbox[1] ) )
		std::cout << "\tBoundbox = { {"
			<< boundbox[0][0] << ", " << boundbox[0][1] << ", " << boundbox[0][2] << "} to {"
			<< boundbox[1][0] << ", " << boundbox[1][1] << ", " << boundbox[1][2] << "} }"
			<< std::endl;
	
	if( const unicode_string* pAuthor = stream.get_metadata_string( "Author" ) )
		std::cout << "\tAuthor = " << *pAuthor << std::endl;
	if( const unicode_string* pAuthor2 = stream.get_metadata_string( "Author_2" ) )
		std::cout << "\tAuthor_2 = " << *pAuthor2 << std::endl;
	
	//We demand a "Position" channel exist, otherwise it throws an exception.
	stream.bind( "Position", theParticle.pos, 3 );

	if( stream.has_channel( "Velocity" ) )
		stream.bind( "Velocity", theParticle.vel, 3 );
	if( stream.has_channel( "Color" ) )
		stream.bind( "Color", theParticle.col, 3 );
	if( stream.has_channel( "Density" ) )
		stream.bind( "Density", &theParticle.density, 1 );
	if( stream.has_channel( "ID" ) )
		stream.bind( "ID", &theParticle.id, 1 );

	std::size_t counter = 0;
	while( stream.read_next_particle() ){
		++counter;

		std::cout << "Particle #" << counter << " w/ ID: " << theParticle.id << '\n';
		std::cout << "\tPosition: [" << theParticle.pos[0] << ", " << theParticle.pos[1] << ", " << theParticle.pos[2] << "]\n";
		std::cout << "\tVelocity: [" << theParticle.vel[0] << ", " << theParticle.vel[1] << ", " << theParticle.vel[2] << "]\n";
		std::cout << "\tColor: [" << theParticle.col[0] << ", " << theParticle.col[1] << ", " << theParticle.col[2] << "]\n";
		std::cout << "\tDensity: " << theParticle.density << '\n';
		std::cout << std::endl;
	}

	stream.close();
}

void example_writing( const std::string& filePath ){
	struct{
		float pos[3];
		float col[3];
		double density;
		unsigned short id;
	} theParticle;

	const int NUM_PARTICLES = 2;

	std::srand( (unsigned)std::time(NULL) );

	prtio::prt_ofstream stream;

	unicode_string author = _T("John Smith");
	unicode_string author2 = _T("Joh\x00F1 \"\x03A6\" Smythe");
	
	prtio::set_coordinate_system( stream, prtio::coordinate_system::right_handed_zup );
	prtio::set_distance_unit( stream, prtio::distance_unit::meters );
	prtio::set_framerate( stream, 30, 1 );
	
	stream.add_metadata( "Author", author );
	stream.add_metadata( "Author_2", author2 );
	//stream.add_metadata( "A\x97thor", author );
	
	stream.bind( "Position", theParticle.pos, 3 );
	stream.bind( "Color", theParticle.col, 3, prtio::data_types::type_float16 ); //Convert it to half on the fly.
	stream.bind( "Density", &theParticle.density, 1 );
	stream.bind( "ID", &theParticle.id, 1 );

	stream.open( filePath );

	for( std::size_t i = 0; i < NUM_PARTICLES; ++i ){
		theParticle.pos[0] = 100.f * ((float)std::rand() / (float)RAND_MAX);
		theParticle.pos[1] = 100.f * ((float)std::rand() / (float)RAND_MAX);
		theParticle.pos[2] = 100.f * ((float)std::rand() / (float)RAND_MAX);

		theParticle.col[0] = (float)(i % 23) / 22.f;
		theParticle.col[1] = (float)((i + 43) % 7) / 6.f;
		theParticle.col[2] = (float)((i + 7) % 91) / 90.f;

		theParticle.density = (double)std::rand() / (double)RAND_MAX + 0.5;
		theParticle.id = (unsigned short)i;

		stream.write_next_particle();
	}

	stream.close();
}

int main( int /*argc*/, char* /*argv*/[] ){
	try{
		std::cout << "Writing to particles_0025.prt" << std::endl;
		example_writing( "particles_0025.prt" );

		std::cout << "Reading from particles_0025.prt" << std::endl;
		example_reading( "particles_0025.prt" );

		return 0;
	}catch( const std::exception& e ){
		std::cerr << std::endl << "ERROR: " << e.what() << std::endl;
		return -1;
	}
}

