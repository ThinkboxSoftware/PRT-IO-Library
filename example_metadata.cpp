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

#if !defined(__APPLE__) && !defined(WIN32) && !defined(_WIN64) && __WORDSIZE == 64
#define INT64 long int
#else
#define INT64 long long
#endif

inline void to_cout( const prtio::prt_meta_value& value ){
#ifdef _WIN32
	// wcout doesn't properly convert non-ASCII charcters on Windows for some reason, so I write directly with WriteConsoleW. Its dumb that wcout doesn't work since that's the whole point of wide chars.
	//value.to_stream( std::wcout, L", " ) << std::flush;

	std::wstringstream ss;
	
	value.to_stream( ss, L", " );

	std::wstring ws = ss.str();

	WriteConsoleW( GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), NULL, NULL );
#else
	value.to_stream( std::cout, ", " ) << std::flush;
#endif
}

void example_reading( const std::string& filePath ){
	struct{
		float pos[3];
		float vel[3];
		float col[3];
		float density;
		INT64 id;
	} theParticle = { {0,0,0}, {0,0,0}, {1.f,1.f,1.f}, 0.f, -1 };

	prtio::prt_ifstream stream( filePath );
	
	if( !stream.get_file_metadata().empty() ){
		std::cout << "File metadata:" << std::endl;
		
		for( std::map< std::string, prtio::prt_meta_value >::const_iterator it = stream.get_file_metadata().begin(), itEnd = stream.get_file_metadata().end(); it != itEnd; ++it ){
			std::cout << "\tName: \"" << it->first << "\" Value: { " << std::flush;
			to_cout( it->second ); 
			std::cout << " }" << std::endl;
		}	
	}
	
	for( std::size_t i = 0, iEnd = stream.layout().num_channels(); i < iEnd; ++i ){
		const std::string& channelName = stream.layout().get_channel_name( i );
		
		const std::map< std::string, prtio::prt_meta_value >& metadata = stream.get_channel_metadata( channelName );
		
		std::cout << "Channel \"" << channelName << "\" metadata:" << std::endl;
		for( std::map< std::string, prtio::prt_meta_value >::const_iterator it = metadata.begin(), itEnd = metadata.end(); it != itEnd; ++it ){
			std::cout << "\tName: \"" << it->first << "\" Value: { " << std::flush;
			to_cout( it->second ); 
			std::cout << " }" << std::endl;
		}
	}
	
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

	const int NUM_PARTICLES = 5;

	std::srand( (unsigned)std::time(NULL) );

	prtio::prt_ofstream stream;
	
	prtio::prt_meta_value author, lengthUnitInMeters, coordinateSystem, arrayExample, arraySubsetExample, jpMessage, frMessage;
	int arrayValues[] = { 42, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29 };
	
#ifdef _WIN32
	author.set_string( L"PRT-IO-LIBRARY\u00A9 Example Program" );
	jpMessage.set_string( L"\u30D1\u30FC\u30D7\u30EB\u30BD\u30C3\u30AF\u30B9" );
	frMessage.set_string( L"votre cuisine est agr\u00E9able" );
#else
	author.set_string( "PRT-IO-LIBRARY\u00A9 Example Program" );
	jpMessage.set_string( "\u30D1\u30FC\u30D7\u30EB\u30BD\u30C3\u30AF\u30B9" );
	frMessage.set_string( "votre cuisine est agr\u00E9able" );
#endif
	lengthUnitInMeters.set( 0.0254 ); // 1 inch = 0.0254 meters.
	coordinateSystem.set( 2 ); // right-handed-z-up (from http://www.thinkboxsoftware.com/krak-prt-11-file-format/)
	arrayExample.set_array( arrayValues );
	arraySubsetExample.set_array( arrayValues+1, 3 ); // Should be 2,3,5 

	stream.add_file_metadata( "author", author );
	stream.add_file_metadata( "LengthUnitInMeters", lengthUnitInMeters );
	stream.add_file_metadata( "CoordSys", coordinateSystem );
	stream.add_file_metadata( "arrayExample", arrayExample );
	stream.add_file_metadata( "arraySubsetExample", arraySubsetExample );
	stream.add_file_metadata( "messageForJapan", jpMessage );
	stream.add_file_metadata( "messageForFrance", frMessage );
	
	prtio::prt_meta_value posInterp;
	posInterp.set( 1 ); // point transform (from http://www.thinkboxsoftware.com/krak-prt-11-file-format/)

	stream.add_channel_metadata( "Position", "Interpretation", posInterp );
	
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
		std::cout << "Writing to particles_0020.prt" << std::endl;
		example_writing( "particles_0020.prt" );

		std::cout << "Reading from particles_0020.prt" << std::endl;
		example_reading( "particles_0020.prt" );

		return 0;
	}catch( const std::exception& e ){
		std::cerr << std::endl << "ERROR: " << e.what() << std::endl;
		return -1;
	}
}

