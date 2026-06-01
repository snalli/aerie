/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _UTIL_STRING_H
#define _UTIL_STRING_H

#include <iostream>
#include <string>

#include <util/types.h>

//============================================================================

namespace util 
{
	/**
	 * Replacement for std::ostringstream. Allows for expressions
	 * such as ostringstream() << "foo" to work as expected.
	 * With the standard ostringstream operator<< for an
	 * integer is invoked because operator<< is only an
	 * external (global) function.
	 * 
	 * NOTE: use oss type (typedefed to ostringstream).
	 */
	class ostringstream : public std::ostringstream
	{
		typedef std::ostringstream super;
	public:
        explicit ostringstream(openmode mode = out);
		explicit ostringstream(std::string const& str, openmode mode = out);

        std::ostream& operator<<(char const* str);
        std::ostream& operator<<(Printable const& obj);

		/**
		 * Returns pointer to the contents of the
		 * stream. 
		 * 
		 * This operator is a hack. It is provided for
		 * backward compatibility for macros that used to take
		 * ostringstream (oss) explicitely in the parameers.
		 * For example:
		 * 
		 * DLOG(1, oss() << "foo" << 123); // old style
		 * DLOG(1, "foo" << "123"); // new style
		 * 
		 * The "new style" should be used, but the "old style" still
		 * has to work and this is achieved thru this conversion.
		 * 
		 * Note, the operator returns pointer to internal data.
		 * Any operation on the stream may invalidate the
		 * returned pointer. In particular this operation is
		 * not thread-safe.
		 * 
		 * Note, for some strange reason
		 * operator<<(ostringstream const&), in member or global
		 * form does not work for unnamed temporaries, e.g.
		 * 
		 * oss tmp, tmp2;
		 * tmp << tmp2; // OKAY
		 * tmp << oss(); // DOES NOT COMPILE!
		 * 	
		 */
		operator char const*() const;

	private:
		mutable std::string strCopy;
	};

	/**
	 * Converts a standard "narrow" string (ASCII) to wide string. 
	 * There are no limits on the length on the
	 * passed strings.
	 *
	 * @param str The source string.
	 *
	 * @return The resulting wide string instance.
	 */
	std::wstring stringToWstring(std::string const& str);
	
	
	/**
	 * Converts a wide string to a single-byte
	 * character string. 
	 * There are no limits on the length on the
	 * passed strings.
	 *
	 * @param str The source string.
	 *
	 * @return The resulting wide string instance.
	 */
	std::string wstringToString(std::wstring const& str);
	

	/**
	 * Converts a sequence of bytes (added with addByte) to a string.
	 */
	class HexConverter 
	{
	public:

	    HexConverter(std::string& str);

		/**
		 * Add to string any character resulting from adding the bits of the argument.
		 * 
		 * @param b
		 */
		void addByte(uint8 b);

		/**
		 * Add any residual character if necessary.
		 */
		void terminate();
		
		/**
		 * The base in which strings are written
		 * Currently, only base 16, 32, or 64 are supported
		 */
		static uint32 const Base;

		static char GetChar(int32 index); // allows indexing up to (2^BITS_PER_CHARACTER - 1)

	private:
		std::string& str;
		uint8 residual;
		uint32  length;
		bool firstByte;

		// Determines the base in which strings are written (4, 5, and 6 bits per char)
		static uint32 const BITS_PER_CHARACTER;

		void again();
	};

	//============================================================================
	//============================================================================

	/**
	 * Concerts string of wide characters to to string of short 
	 * characters.
	 * 
	 * @deprecated 
	 */
	void wcharTochar(wchar const* str, char *buf);
	
	/**
	 * Concerts string of short characters to to string of wide
	 * characters.
	 * 
	 * @deprecated 
	 */
	void charToWchar(char const* str, wchar *buf);
			
	/**
	 * Converts ostream to string if possible (ostream must
	 * be stringstream or ostringstream).
	 * 
	 * @param os     An output stream.
	 * @param ok     Flag indeIt is modified regardles of the return
	 *               value.
	 * 
	 * @return Resulting string.
	 * @deprecated 
	 */
	std::string ostreamToString(std::ostream const& os, bool *ok = NULL);

	/**
	 * Converts wostream to wstring if possible (wostream must
	 * be wstringstream or wostringstream).
	 * 
	 * @param wos
	 * @param ok     Flag indeIt is modified regardles of the return
	 *               value.
	 * 
	 * @return Resulting string.
	 * @deprecated 
	 */
	std::wstring wostreamToWstring(std::wostream const& wos, bool *ok = NULL);


} // end of namespace util

//============================================================================

#endif 
