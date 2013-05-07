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

/**
 * This class is designed to be a type-safe way to hold a value of any type. The template function get() will throw exceptions if the held type does not match
 * the stored type, while the get_ptr() function will return NULL if the stored type does not match.
 */
class any{
public:
	/**
	 * Default constructor leaves the object empty (ie. this->empty() returns true).
	 */
	any();
	
	/**
	 * Copies an any object by doing a deep copy of the contained value
	 */
	any( const any& rhs );
	
	/**
	 * Copies an any object by doing a deep copy of the contained value
	 */
	any& operator=( const any& rhs );
	
	/**
	 * Returns true if the has not had a value assigned.
	 */
	bool empty() const;
	
	/**
	 * Assigns a new value to the object. A copy of the passed object is made.
	 * \tparam T Any type with a valid copy constructor.
	 * \param val The new value to copy and store.
	 */
	template <class T>
	void set( const T& val );
	
	/**
	 * Interprets the stored object as a T and returns a reference to it. Throws a any::bad_cast exception if the stored value is not actually a T.
	 * \pre this->empty() == false
	 * \tparam T The type to retrieve the object as. Must exactly match the type used with a previous call to set().
	 * \return A reference to the contained value.
	 */
	template <class T>
	T& get();
	
	/**
	 * \overload This is a const version of get()
	 */
	template <class T>
	const T& get() const;
	
	/**
	 * Interprets the stored object as a T and returns a pointer to it. Returns a NULL pointer if the stored value is not a T.
	 * \tparam T the type to retrieve the object as.
	 * \return A pointer to the contained T value, or NULL if a T value is not held.
	 */
	template <class T>
	T* get_ptr();
	
	/**
	 * \overload
	 */
	template <class T>
	const T* get_ptr() const;
	
	/**
	 * Swaps the contained values of two any objects.
	 */
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
	
	/**
	 * The type thrown when get<T>() is called and the requested type does not match the held type.
	 */
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

/**
 * This swap function should be found via ADL when called without explicit namespaces on any objects.
 */
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

