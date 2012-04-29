/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

#include <stdarg.h>
#include <wchar.h>
#include <vector>
#include <stdio.h>
#include <string.h>

#include <util/string.h>

using std::string;
using std::wstring;
using std::ostream;
using std::wostream;

namespace util
{
	ostringstream::ostringstream(openmode mode): super(mode)
	{
	}

	ostringstream::ostringstream(string const& str, openmode mode): super(str, mode)
	{
	}
    
	ostream& ostringstream::operator<<(char const* str)
	{
		return std::operator<<(*this, str);
	}

	ostream& ostringstream::operator<<(Printable const& obj)
	{
		return obj.print(*this);
	}

	ostringstream::operator char const*() const
	{
		string const tmp = this->str();

		if (tmp.empty())
		{
			return "";
		}
		else
		{
			this->strCopy = tmp;
			return this->strCopy.c_str();
		}
	}



	//============================================================================
	
	static wchar *getSafeBuffer(wchar* stackBuf,
								int32 stackBufSize,
								int32 reqBufSize,
								wchar** freeHandle)
	{
		if (reqBufSize <= stackBufSize)
		{
			wchar* newBuf = new wchar[reqBufSize];
			*freeHandle = newBuf;
			return newBuf;
		}
	
		*freeHandle = NULL;
		return stackBuf;
	}
	
	static char *getSafeBuffer(char* stackBuf,
							   int32 stackBufSize,
							   int32 reqBufSize,
							   char** freeHandle)
	{
		if (reqBufSize <= stackBufSize)
		{
			char* newBuf = new char[reqBufSize];
			*freeHandle = newBuf;
			return newBuf;
		}
	
		*freeHandle = NULL;
		return stackBuf;
	}
	
	static void freeSafeBuffer(wchar *freeHandle)
	{
		if (freeHandle) // I know delete is ok with NULL, this is an optimization
			delete freeHandle;
	}
	
	static void freeSafeBuffer(char *freeHandle)
	{
		if (freeHandle) // I know delete is ok with NULL, this is an optimization
			delete freeHandle;
	}
	
	//============================================================================
	
	wstring stringToWstring(string const& str)
	{
		wchar buf[SAFE_STR_LEN], *freeHandle;
		int32 len = str.size();
			
		wchar *p = getSafeBuffer(buf, sizeof(buf), len + 1, &freeHandle);		
		swprintf(p, len + 1, L"%s", str.c_str());
		wstring result = p;
	
		freeSafeBuffer(freeHandle);
		return result;
	}
		
	string wstringToString(wstring const& str)
	{
		char buf[SAFE_STR_LEN], *freeHandle;
		int32 len = str.size();
			
		char *p = getSafeBuffer(buf, sizeof(buf), len + 1, &freeHandle);
		sprintf(p, "%ls", str.c_str());
		string result = p;
	
		freeSafeBuffer(freeHandle);
	
		return result;
	}
	
	void wcharTochar(wchar const* str, char *buf)
	{
		sprintf(buf, "%S", str);
	}
	
	void charToWchar(char const* str, wchar* buf)
	{
		swprintf(buf, strlen(str) + 1, L"%S", str);
	}	
	
	//============================================================================
	
	string ostreamToString(ostream const& os, bool *ok)
	{
		if (std::ostringstream const* oss = dynamic_cast<std::ostringstream const*>(&os))
		{
			ok && (*ok = true);
			return oss->str();
		}
		else if (std::stringstream const* oss = dynamic_cast<std::stringstream const*>(&os))
		{
			ok && (*ok = true);
			return oss->str();
		}
		else
		{
			ok && (*ok = false);
			return "<non-stringstream passed>";
		}
	}

    wstring wostreamToWstring(wostream const& os, bool *ok)
	{
		if (std::wostringstream const* oss = dynamic_cast<std::wostringstream const*>(&os))
		{
			ok && (*ok = true);
			return oss->str();
		}
		else if (std::wstringstream const* oss = dynamic_cast<std::wstringstream const*>(&os))
		{
			ok && (*ok = true);
			return oss->str();
		}
		else
		{
			ok && (*ok = false);
			return L"<non-stringstream passed>";
		}
	}

	//============================================================================

	uint32 const HexConverter::BITS_PER_CHARACTER = 6;  // base 64
	uint32 const HexConverter::Base = (0x1 << BITS_PER_CHARACTER);

	static char const CharTable[]  = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
		'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '+', '=',
		':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':',
		':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':',
		':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':',
		':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':', ':'
	};
	
	char HexConverter::GetChar( int32 index)
	{
		static uint32 const TABLE_MASK = 0xff >> (BITS_PER_BYTE - BITS_PER_CHARACTER);
		return CharTable[ index & TABLE_MASK];
	}

	HexConverter::HexConverter( string & str)
		: str( str), residual( 0), length( 0), firstByte( true)
	{
	}


	void HexConverter::addByte( uint8 b)
	{
		if( this->firstByte)
		{
			uint32 half = (BITS_PER_BYTE / 2);
			this->str.push_back( GetChar( b >> half));
			this->str.push_back( GetChar( 0x0f & b));
//  			this->str.push_back( '/');
			this->firstByte = false;
			return;
		}

  		// Start from residual (if any)
  		uint32 current = this->residual;
  		// Set new residual value
  		this->residual = b;
  		// The number of positions that have to be completed from the new byte
  		uint32 newPositions = (BITS_PER_CHARACTER - this->length);
  		// Number of "residual" positions (next length)
  		this->length = BITS_PER_BYTE - newPositions;
  		// Make room to the right for newPositions bits
  		current <<= newPositions;
  		// keep the first newPositions of b, right-aligned. OR with current
  		current |= (b >> this->length);
  		this->str.push_back( GetChar( current));
		
  		this->again();
  	}

	void HexConverter::again()
	{
		if( this->length < BITS_PER_CHARACTER)
		{
			return;
		}
		uint32 current = this->residual >> (this->length - BITS_PER_CHARACTER);
		// residual remains the same, but length is shortened
		this->length -= BITS_PER_CHARACTER;
		this->str.push_back(  GetChar( current));
		again();
	}

	void HexConverter::terminate()
	{
		uint32 current = this->residual;
		current <<= BITS_PER_CHARACTER;
		this->str.push_back( GetChar( current >> BITS_PER_CHARACTER));
	}
} // end of namespace util
