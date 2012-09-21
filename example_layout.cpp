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

void layout_reading( const std::string& filePath ){
	struct{
		float pos[3];
		float vel[3];
		float col[3];
		float density;
		INT64 id;
	} theParticle = { {0,0,0}, {0,0,0}, {1.f,1.f,1.f}, 0.f, -1 };

	prtio::prt_ifstream stream( filePath );

	const prtio::prt_layout& layout = stream.layout();

	size_t numChannels = layout.num_channels();
	for (size_t i=0;i<numChannels;i++)
	{
		std::cout << "Channel name found = "
				<< layout.get_channel_name(i).c_str()
				<< std::endl;
		const prtio::detail::prt_channel& channel = layout.get_channel(layout.get_channel_name(i));
		std::cout << "Channel arity = " << channel.arity << std::endl;
		std::cout << "Channel offset = " << channel.offset << std::endl;
		std::string type_name;
		switch (channel.type)
		{
		case prtio::data_types::type_int16 :
			type_name = "int16";
			break;
		case prtio::data_types::type_int32 :
			type_name = "int32";
			break;
		case prtio::data_types::type_int64 :
			type_name = "int64";
			break;
		case prtio::data_types::type_float16 :
			type_name = "float16";
			break;
		case prtio::data_types::type_float32 :
			type_name = "float32";
			break;
		case prtio::data_types::type_float64 :
			type_name = "float64";
			break;
		case prtio::data_types::type_uint16 :
			type_name = "uint16";
			break;
		case prtio::data_types::type_uint32 :
			type_name = "uint32";
			break;
		case prtio::data_types::type_uint64 :
			type_name = "uint64";
			break;
		case prtio::data_types::type_int8 :
			type_name = "int8";
			break;
		case prtio::data_types::type_uint8 :
			type_name = "uint8";
			break;
		default:
			type_name = "UNKNOWN CHANNEL TYPE";
			break;
		}
		std::cout << "Channel type = " << type_name.c_str() << std::endl;
	}
	stream.close();
}

void layout_writing( const std::string& filePath ){
	struct{
		float pos[3];
		float col[3];
		double density;
		unsigned short id;
	} theParticle;

	const int NUM_PARTICLES = 791;

	std::srand( (unsigned)std::time(NULL) );

	prtio::prt_ofstream stream;

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
		layout_writing( "particles_0020.prt" );

		std::cout << "Reading from particles_0020.prt" << std::endl;
		layout_reading( "particles_0020.prt" );

		return 0;
	}catch( const std::exception& e ){
		std::cerr << std::endl << "ERROR: " << e.what() << std::endl;
		return -1;
	}
}

