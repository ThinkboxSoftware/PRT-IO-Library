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
 * This file contains the implementation details of class any. See <prtio/detail/any.hpp> for more information.
 */
#pragma once

#include <prtio/detail/data_types.hpp>

#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace prtio{ namespace detail{

/**
 * Abstract class definine the interface that is held by any objects.
 */
class any::any_interface{
public:
	virtual ~any_interface(){}
	virtual const std::type_info& get_type() const = 0;
	virtual void* get() = 0;
	virtual const void* get() const = 0;
	virtual any_interface* clone() const = 0;
	virtual void write( std::ostream& ostream ) const = 0;
};

/**
 * Template class providing concrete implementations of any_interface for use in any objects. This class
 * holds an actual T instance for an any object.
 * \tparam T Must implement a copy constructor, and be supported by template class write_any_helper.
 */
template <typename T>
class any::any_impl : public any::any_interface{
public:
	any_impl( const T& val );
	
	virtual const std::type_info& get_type() const;
	virtual void* get();
	virtual const void* get() const;
	virtual any::any_interface* clone() const;
	virtual void write( std::ostream& ostream ) const;
	
private:
	T m_val;
};

template <typename T>
inline any::any_impl<T>::any_impl( const T& val ) 
	: m_val( val )
{}

template <typename T>
inline const std::type_info& any::any_impl<T>::get_type() const {
	return typeid(T);
}

template <typename T>
inline void* any::any_impl<T>::get(){
	return &m_val;
}

template <typename T>
inline const void* any::any_impl<T>::get() const {
	return &m_val;
}

template <typename T>
inline any::any_interface* any::any_impl<T>::clone() const {
	return new any_impl( m_val );
}

template <typename T>
struct write_any_helper;

template <typename T>
struct write_any_helper< std::vector<T> >{
	inline static void write( std::ostream& ostream, const std::vector<T>& value ){
		data_types::int32_t type = data_types::traits<T>::data_type();
		data_types::int32_t arity = static_cast<data_types::int32_t>( value.size() );
		
		ostream.write( reinterpret_cast<const char*>( &type ), 4 );
		ostream.write( reinterpret_cast<const char*>( &arity ), 4 );
		ostream.write( reinterpret_cast<const char*>( &value.front() ), sizeof(T) * value.size() );
	}
};

#ifdef _WIN32
template <>
struct write_any_helper< std::wstring >{
	inline static void write( std::ostream& ostream, const std::wstring& value ){
		std::string narrowVal;
		
		// Don't include the null char when converting since std::string::c_str() will add it for me later.
		int r = WideCharToMultiByte( CP_UTF8, 0, value.c_str(), static_cast<int>( value.size() ), NULL, 0, NULL, NULL );
		if( r > 0 ){
			narrowVal.assign( r, '\0' );
			
			r = WideCharToMultiByte( CP_UTF8, 0, value.c_str(), static_cast<int>( value.size() ), &narrowVal[0], r, NULL, NULL );
		}
		
		if( r == 0 )
			throw std::runtime_error( "Failed to convert metadata string value to UTF-8" );
	
		data_types::int32_t type = -1;
		data_types::int32_t arity = static_cast<data_types::int32_t>( narrowVal.size() + 1 );
		
		ostream.write( reinterpret_cast<const char*>( &type ), 4 );
		ostream.write( reinterpret_cast<const char*>( &arity ), 4 );
		ostream.write( narrowVal.c_str(), arity ); // Make sure to write the trailing NULL character.
	}
};
#else
template <>
struct write_any_helper< std::string >{
	inline static void write( std::ostream& ostream, const std::string& value ){
		data_types::int32_t type = -1;
		data_types::int32_t arity = static_cast<data_types::int32_t>( value.size() + 1 );
		
		ostream.write( reinterpret_cast<const char*>( &type ), 4 );
		ostream.write( reinterpret_cast<const char*>( &arity ), 4 );
		ostream.write( value.c_str(), arity ); // Make sure to write the trailing NULL character.
	}
};
#endif

template <typename T>
inline void any::any_impl<T>::write( std::ostream& ostream ) const {
	write_any_helper<T>::write( ostream, m_val );
}

inline any::any() 
{}

inline any::any( const any& rhs )
	: m_pImpl( rhs.m_pImpl.get() ? rhs.m_pImpl->clone() : NULL )
{}

inline any& any::operator=( const any& rhs ){
	m_pImpl.reset( rhs.m_pImpl.get() ? rhs.m_pImpl->clone() : NULL );
}

inline bool any::empty() const {
	return m_pImpl.get() == NULL;
}

template <typename T>
inline void any::set( const T& val ){
	m_pImpl.reset( new any_impl<T>( val ) );
}

template <typename T>
inline T& any::get(){
	if( !m_pImpl.get() || m_pImpl->get_type() != typeid(T) )
		throw bad_cast( typeid(T), m_pImpl->get_type() );
	return *static_cast<T*>( m_pImpl->get() );
}

template <typename T>
inline const T& any::get() const {
	if( !m_pImpl.get() || m_pImpl->get_type() != typeid(T) )
		throw bad_cast( typeid(T), m_pImpl->get_type() );
	return *static_cast<const T*>( m_pImpl->get() );
}

template <typename T>
inline T* any::get_ptr(){
	if( !m_pImpl.get() || m_pImpl->get_type() != typeid(T) )
		return NULL;
	return static_cast<T*>( m_pImpl->get() );
}
	
template <typename T>
inline const T* any::get_ptr() const {
	if( !m_pImpl.get() || m_pImpl->get_type() != typeid(T) )
		return NULL;
	return static_cast<const T*>( m_pImpl->get() );
}

inline void any::swap( any& rhs ){
	//std::swap( m_pImpl, rhs.m_pImpl );
	std::auto_ptr<any_interface> t = m_pImpl;
	m_pImpl = rhs.m_pImpl;
	rhs.m_pImpl = t;
}

inline void any::write( std::ostream& ostream ) const {
	if( !m_pImpl.get() )
		throw std::runtime_error( "Invalid attempt to write an empty any object" );
	m_pImpl->write( ostream );
}

} }
