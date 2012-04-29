/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 * 
 * Common types.
 */
#ifndef _UTIL_TYPES_H
#define _UTIL_TYPES_H

#include <wchar.h>
#include <limits.h>
#include <stdint.h>

#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <boost/scoped_array.hpp>
#include <stdint.h>
#include <typeinfo>

namespace util
{

	//============================================================================
	// Primitive types
	//============================================================================

	typedef short word;
	typedef unsigned short uword;
	typedef uint8_t ubyte;
	typedef uint8_t byte;
	#ifndef HAVE_UCHAR
	typedef unsigned char uchar;
	#endif
	typedef wchar_t wchar;
	// Avoid conflict with uint32 in sys/types.h
	//typedef unsigned int uint;

	typedef int8_t int8;
	typedef int16_t int16;
	typedef int32_t int32;
	typedef long long int64;
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef unsigned long long uint64;

	//============================================================================
	// Complex types
	//============================================================================

	/// Set of strings
	typedef std::set<std::string> StringSet;
	/// Map - strings->strings
	typedef std::map<std::string, std::string> StringMap;
	/// Vectors of strings
	typedef std::vector<std::string> StringVector;

	class ostringstream;

	typedef util::ostringstream oss;
	typedef std::istringstream iss;

	/**
	 * See http://www.boost.org/libs/smart_ptr/scoped_array.htm for documentation
	 */
	typedef boost::scoped_array<byte> ScopedBuffer;

	//============================================================================
	// Magic values
	//============================================================================

	enum
	{
		BITS_PER_BYTE = 8,
		SAFE_STR_LEN = 1024,
		MAX_FILEPATH_LEN = 1024
	};

	/**
	 * Printable interface. This is actually an abstract class that
	 * provides default print implementation.
	 */
	class Printable
	{
	public:
		virtual ~Printable() = 0;

		virtual std::ostream& print(std::ostream& s) const;

		/**
		 * Returns object's printed state as string.
		 * This is just a convenience function which uses method operating on stream.
		 * Do not reimplement - reimplement print(std::ostream&).
		 */
		std::string printStr() const;
	};



	//============================================================================
	//
	// IMPLEMENTATON
	//
	//============================================================================

	inline Printable::~Printable()
	{
	}

	inline std::ostream& Printable::print(std::ostream& s) const
	{
		std::ios_base::fmtflags orgFlags = s.flags();
		s << "{" << typeid(*this).name() << ":" << std::showbase << std::hex << reinterpret_cast<uintptr_t>(this) << "}";
		s.flags(orgFlags);
		return s;
	}

	inline std::ostream& operator<<(std::ostream& strm, Printable const& object)
	{
		object.print(strm);
		return strm;
	}

	inline std::string Printable::printStr() const
	{
		std::ostringstream strm;
		strm << (*this);
		return strm.str();
	}

} // end of namespace util

#endif // _UTIL_TYPES_H

