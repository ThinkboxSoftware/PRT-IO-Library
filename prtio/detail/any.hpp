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
 * This file contains class any, an object that holds data in a truly variable manner while remaining typesafe. It is patterned 
 * after the Boost.Any library (http://www.boost.org/doc/libs/1_53_0/doc/html/any.html).
 */
#pragma once

#include <exception>
#include <memory>
#include <ostream>
#include <utility>

namespace prtio{ namespace detail{

class any{
public:
	any();
	
	any( const any& rhs );
	
	any& operator=( const any& rhs );
	
	bool empty() const;
	
	template <class T>
	void set( const T& val );
	
	template <class T>
	T& get();
	
	template <class T>
	const T& get() const;
	
	template <class T>
	T* get_ptr();
	
	template <class T>
	const T* get_ptr() const;
	
	void swap( any& rhs );
	
	/** 
	 * Will write the data type, arity, and actual value to the provided stream. The data type and arity are written as
	 * 32 bit integers.
	 * \note: The format is:
	 *         (4 bytes) data type
	 *         (4 bytes) arity
	 *         ('sizeof(data type) * arity' bytes) value
	 * \param ostream The stream to write to.
	 */
	void write( std::ostream& ostream ) const;
	
	class bad_cast;
	
private:
	class any_interface;
	
	template <typename T> class any_impl;
	
	std::auto_ptr<any_interface> m_pImpl;
};

class any::bad_cast : public std::exception{
public:
	bad_cast( const std::type_info& /*castType*/, const std::type_info& /*storedType*/ )
	{}
	
	virtual const char* what() const {
		return "Attempted to get the wrong type from an any object";
	}
};

inline void swap( any& lhs, any& rhs ){
	lhs.swap( rhs );
}

} }

namespace std{
	template <>
	inline void swap<prtio::detail::any>( prtio::detail::any& lhs, prtio::detail::any& rhs ){
		lhs.swap( rhs );
	}
}

#include <prtio/detail/any_impl.hpp>

