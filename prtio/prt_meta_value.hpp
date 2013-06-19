#pragma once

#include <prtio/detail/data_types.hpp>

#include <cassert>
#include <cstring>
#include <new>
#include <string>

namespace prtio{

// We store strings UTF8 encoded, but on Windows we work with them as UTF16LE in memory. The conversion is automatic at the time of file I/O.
#ifdef _WIN32
typedef wchar_t uchar_type;
#else
typedef char uchar_type;
#endif

namespace meta_types{
	enum option{
		type_string = -1, // A unicode, NULL terminated string stored on disk as UTF8, and in memory as UTF8 on Mac/Linux or UTF16LE on Windows
		type_int8 = data_types::type_int8,
		type_int16 = data_types::type_int16,
		type_int32 = data_types::type_int32,
		type_int64 = data_types::type_int64,
		type_uint8 = data_types::type_uint8,
		type_uint16 = data_types::type_uint16,
		type_uint32 = data_types::type_uint32,
		type_uint64 = data_types::type_uint64,
		type_float16 = data_types::type_float16,
		type_float32 = data_types::type_float32,
		type_float64 = data_types::type_float64,
		type_last = data_types::type_count, // Marks the last position in sequential order. Not labelled the same since the count is actually type_last + 1 due to some negative valued types.
		type_invalid = 127
	};
}

/**
 * Type for storing the values retrieved from the metadata sections of a PRT v1.1 file. Provides generic & type-safe access to the stored value(s).
 */
class prt_meta_value{
public:
	prt_meta_value();
	
	prt_meta_value( const prt_meta_value& rhs );
	
	~prt_meta_value();
	
	prt_meta_value& operator=( const prt_meta_value& rhs );

	meta_types::option get_type() const;
	
	std::size_t get_arity() const;
	
	/**
	 * True if the value has been assigned, false if it was default constructed.
	 */
	bool is_valid() const;
	
	/**
	 * Typed accessor, returns a reference to the held value of the specified type. Throws if the specified type is not held. It is specialized to handle finite array types
	 * in order to implement arity checking as well.
	 *
	 * Ex. get<float>() is valid for float32[1] and returns the scalar value.
	 * Ex. get<float[3]>() is valid for float32[3] and returns a reference to the 3 float array held internally.
	 */
	template <class T>
	T& get();
	template <class T>
	const T& get() const;
	
	/**
	 * Typed accessor that returns a pointer to the data in memory. Returns NULL if the specified type is not held. Use prt_meta_value::get_arity() to determine the 
	 * array's length.
	 */
	template <class T>
	T* get_ptr();
	template <class T>
	const T* get_ptr() const;
	
	/**
	 * Untyped accessor returns a pointer to the data in memory. Use prt_meta_value::get_type() and prt_meta_value::get_arity() to determine what to do with the data.
	 */
	void* get_void_ptr();
	const void* get_void_ptr() const;
	
	/**
	 * Accessor for string values. It is returned as a pointer to a NULL terminated C-style string.
	 */
	const uchar_type* get_string() const;
	
	/**
	 * Typed setter, sets the value to a T[1] array.
	 */
	template <class T>
	void set( T value );
	
	/**
	 * Typed setter for an array.
	 */
	template <class T, int Length>
	void set_array( const T (&array)[Length] );
	
	/**
	 * Typed setter for an array recieved via pointer.
	 */	 
	template <class T>
	void set_array( const T* array, std::size_t length );
	
	/**
	 * Setter for string value.
	 */
	void set_string( const uchar_type* pStr );
	
	/**
	 * Setter for string value, with an explicit length. The length is in characters (ie. not bytes) and does not include the NULL terminator.
	 */
	void set_string( const uchar_type* pStr, std::size_t length );
	
	/**
	 * \overload for std::string/std::wstring as appropriate for the platform.
	 */
	void set_string( const std::basic_string<uchar_type>& str ){ this->set_string( str.c_str(), str.size() ); }
	
	/**
	 * Generic setter.
	 * \type The type of the data pointed at by 'pData'
	 * \arity The arity of the data pointed at by 'pData'. It should be 1 for string values.
	 * \pData Pointer to the data. For string types pass the actual C string pointer, for numeric types pass a pointer to the first element.
	 */
	void set( meta_types::option type, std::size_t arity, const void* pData );
	
	/**
	 * Prints the contents of this object to the stream as a string with each value separated by the indicated string.
	 */
	std::basic_ostream<uchar_type>& to_stream( std::basic_ostream<uchar_type>& stream, const std::basic_string<uchar_type>& separator ) const;
	
private:
	template <class T>
	void set_internal( std::size_t arity, const void* pData );
	
	template <class T>
	std::basic_ostream<uchar_type>& to_stream_internal( std::basic_ostream<uchar_type>& stream, const std::basic_string<uchar_type>& separator ) const;
	
	void reset( meta_types::option type, std::size_t arity, void* pData );
	
private:
	meta_types::option m_type;
	std::size_t m_arity;
	void* m_pData;
};

inline prt_meta_value::prt_meta_value()
	: m_pData( NULL ), m_type( meta_types::type_invalid ), m_arity( 0 )
{}

inline prt_meta_value::prt_meta_value( const prt_meta_value& rhs )
	: m_pData( NULL ), m_type( meta_types::type_invalid ), m_arity( 0 )
{
	if( rhs.is_valid() )
		this->set( rhs.m_type, rhs.m_arity, rhs.m_pData );
}

inline prt_meta_value::~prt_meta_value(){
	operator delete( m_pData );
}

inline prt_meta_value& prt_meta_value::operator=( const prt_meta_value& rhs ){
	if( rhs.is_valid() ){
		this->set( rhs.m_type, rhs.m_arity, rhs.m_pData );
	}else{
		this->reset( meta_types::type_invalid, 0, NULL );
	}

	return *this;
}

template <class T>
inline void prt_meta_value::set_internal( std::size_t arity, const void* pData ){
	this->reset( static_cast<meta_types::option>( data_types::traits<T>::data_type() ), arity, operator new( sizeof(T) * arity ) );
	std::memcpy( m_pData, pData, sizeof(T) * arity );
}

template <class T>
inline std::basic_ostream<uchar_type>& prt_meta_value::to_stream_internal( std::basic_ostream<uchar_type>& stream, const std::basic_string<uchar_type>& separator ) const {
	stream << static_cast<T*>(m_pData)[0];
	for( std::size_t i = 1; i < m_arity; ++i )
		stream << separator << static_cast<T*>(m_pData)[i];
	return stream;
}

inline void prt_meta_value::reset( meta_types::option type, std::size_t arity, void* pData ){
	if( m_pData )
		operator delete( m_pData );
	m_pData = pData;
	m_type = type;
	m_arity = arity;
}

inline void prt_meta_value::set( meta_types::option type, std::size_t arity, const void* pData ){
	assert( pData != NULL );
	
	switch( type ){
	case meta_types::type_float16:
		this->set_internal<data_types::float16_t>( arity, pData );
		break;
	case meta_types::type_float32:
		this->set_internal<data_types::float32_t>( arity, pData );
		break;
	case meta_types::type_float64:
		this->set_internal<data_types::float64_t>( arity, pData );
		break;
	case meta_types::type_int8:
		this->set_internal<data_types::int8_t>( arity, pData );
		break;
	case meta_types::type_int16:
		this->set_internal<data_types::int16_t>( arity, pData );
		break;
	case meta_types::type_int32:
		this->set_internal<data_types::int32_t>( arity, pData );
		break;
	case meta_types::type_int64:
		this->set_internal<data_types::int64_t>( arity, pData );
		break;
	case meta_types::type_uint8:
		this->set_internal<data_types::uint8_t>( arity, pData );
		break;
	case meta_types::type_uint16:
		this->set_internal<data_types::uint16_t>( arity, pData );
		break;
	case meta_types::type_uint32:
		this->set_internal<data_types::uint32_t>( arity, pData );
		break;
	case meta_types::type_uint64:
		this->set_internal<data_types::uint64_t>( arity, pData );
		break;
	case meta_types::type_string:
		assert( arity == 1 );
		
		this->set_string( static_cast<const uchar_type*>( pData ) );
		break;
	default:
		break;
	}
}

template <class T>
inline void prt_meta_value::set( T value ){
	this->set_internal<T>( 1u, &value );
}

template <class T, int Length>
inline void prt_meta_value::set_array( const T (&values)[Length] ){
	this->set_internal<T>( Length, &values[0] );
}

template <class T>
inline void prt_meta_value::set_array( const T* values, std::size_t length ){
	this->set_internal<T>( length, values );
}

inline void prt_meta_value::set_string( const uchar_type* pStr ){
	this->set_string( pStr, std::char_traits<uchar_type>::length( pStr ) );
}

inline void prt_meta_value::set_string( const uchar_type* pStr, std::size_t length ){
	this->reset( meta_types::type_string, 1u, operator new( (length+1) * sizeof(uchar_type) ) );
	
	std::char_traits<uchar_type>::copy( static_cast<uchar_type*>( m_pData ), static_cast<const uchar_type*>( pStr ), length+1 );
	
	// Make sure we have a terminating NULL character.
	static_cast<uchar_type*>( m_pData )[length] = static_cast<uchar_type>( 0 );
}

namespace detail{
	template <class T>
	struct get_impl{
		inline static T& apply( meta_types::option type, std::size_t arity, void* pData ){
			if( type != data_types::traits<T>::data_type() || arity != 1u )
				throw std::runtime_error( "Invalid type access" );
			return static_cast<T*>( pData )[0];
		}
	};
	
	template <class T, int Length>
	struct get_impl<T[Length]>{
		typedef T(&reference_type)[Length];
		
		inline static reference_type apply( meta_types::option type, std::size_t arity, void* pData ){
			if( type != data_types::traits<T>::data_type() || arity != Length )
				throw std::runtime_error( "Invalid type access" );
			return *static_cast<T(*)[Length]>( pData );
		}
	};
	
	template <>
	struct get_impl<const uchar_type*>{
		inline static const uchar_type* apply( meta_types::option type, std::size_t arity, void* pData ){
			if( type != meta_types::type_string || arity != 1u )
				throw std::runtime_error( "Invalid type access" );
			return static_cast<const uchar_type*>( pData );
		}
	};
}

template <class T>
inline T& prt_meta_value::get(){
	return get_impl<T>::apply( m_type, m_arity, m_pData );
}

template <class T>
inline const T& prt_meta_value::get() const {
	return get_impl<T>::apply( m_type, m_arity, m_pData );
}


template <class T>
inline T* prt_meta_value::get_ptr(){
	if( data_types::traits<T>::data_type() != m_type )
		return NULL;
	return static_cast<T*>( m_pData );
}

template <class T>
inline const T* prt_meta_value::get_ptr() const {
	if( data_types::traits<T>::data_type() != m_type )
		return NULL;
	return static_cast<T*>( m_pData );
}

inline const uchar_type* prt_meta_value::get_string() const {
	if( meta_types::type_string != m_type )
		return NULL;
	return static_cast<const uchar_type*>( m_pData );
}

inline void* prt_meta_value::get_void_ptr(){
	return m_pData;
}

inline const void* prt_meta_value::get_void_ptr() const{
	return m_pData;
}

inline meta_types::option prt_meta_value::get_type() const {
	return m_type;
}

inline std::size_t prt_meta_value::get_arity() const {
	return m_arity;
}

inline bool prt_meta_value::is_valid() const {
	return m_type != meta_types::type_invalid && m_arity != 0 && m_pData != NULL;
}

inline std::basic_ostream<uchar_type>& prt_meta_value::to_stream( std::basic_ostream<uchar_type>& stream, const std::basic_string<uchar_type>& separator ) const {
	switch( m_type ){
	case meta_types::type_float16:
		return this->to_stream_internal<data_types::float16_t>( stream, separator );
	case meta_types::type_float32:
		return this->to_stream_internal<data_types::float32_t>( stream, separator  );
	case meta_types::type_float64:
		return this->to_stream_internal<data_types::float64_t>( stream, separator  );
	case meta_types::type_int8:
		return this->to_stream_internal<data_types::int8_t>( stream, separator  );
	case meta_types::type_int16:
		return this->to_stream_internal<data_types::int16_t>( stream, separator  );
	case meta_types::type_int32:
		return this->to_stream_internal<data_types::int32_t>( stream, separator  );
	case meta_types::type_int64:
		return this->to_stream_internal<data_types::int64_t>( stream, separator  );
	case meta_types::type_uint8:
		return this->to_stream_internal<data_types::uint8_t>( stream, separator  );
	case meta_types::type_uint16:
		return this->to_stream_internal<data_types::uint16_t>( stream, separator  );
	case meta_types::type_uint32:
		return this->to_stream_internal<data_types::uint32_t>( stream, separator  );
	case meta_types::type_uint64:
		return this->to_stream_internal<data_types::uint64_t>( stream, separator  );
	case meta_types::type_string:
		return stream << static_cast<uchar_type*>( m_pData );
	default:
		return stream;
	}
}

}
