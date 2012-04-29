/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _UTIL_MISC_H
#define _UTIL_MISC_H
/**
 * Miscellaneous classes.
 */

#include <set>
#include <map>
#include <sys/ioctl.h>
#include <stdio.h>

#if BOOST_VERSION >= 103300
// boost::none seems more readable and can avoid a default construction
#include <boost/none.hpp>
#endif

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/version.hpp>

#include "util/exceptions.h"
#include "util/debug.h"

#ifndef NULL
#define NULL    0
#endif

/**
 * Make a string from an expression.
 */
#define MAKE_STR(s) MAKE_STR1(s)
#define MAKE_STR1(s) #s
//============================================================================

namespace util
{
#define EQUALITY_COMPARABLE(ClassName)									\
  inline bool operator!=(ClassName const & x, ClassName const & y) { return !(x == y); }

#define LESS_THAN_COMPARABLE(ClassName)									\
  inline bool operator>(ClassName const & x,ClassName const & y)  { return y < x; }	\
  inline bool operator<=(ClassName const & x,ClassName const & y) { return !(y < x); } \
  inline bool operator>=( ClassName const & x,ClassName const & y) { return !(x < y); }

  template <class T>
  inline void unused(T const&)
  {
  }

  //============================================================================

  /**
   * Convert a string byte size into a 64-bit integer.
   *
   * The string represents a numeric byte size and consists of numbers, an
   * optional decimal point followed by an optional decimal, and ending with an
   * optional suffix indicating a multiplier.  The multiplier is case-insensitive
   * and supports 'k' for kilobyte, 'm' for megabyte, 'g' for gigabyte, and 't'
   * for terabyte.  The suffix can optionally end with a 'b', indicating "byte,"
   * such as "kb" for kilobyte.
   *
   * The default multiplier is 1024.  Callers can override this by providing an
   * alternate value, for example 1000.  There is no way to change the meaning
   * of each letter suffix, so using more than one multiplier when processing
   * strings from the same file can be confusing.
   *
   * White space between the numeric portion of the string and the suffix is
   * optional and ignored.  Conversion ends when either the end of the string is
   * reached, or the first character is encountered that doesn't conform to the
   * described syntax.
   *
   * Examples:
   * - If any of the strings "2147483648", "2 g", "2 G", "2 gb", "2 GB",
   *   "2.0 GB", "2g", "2GB" is passed to byteStringToNumber(), the return
   *   value will be 2147483648.
   * - The string "1.5 GB" will result in a return value of 1610612736.
   *
   * Throws ParseException if the string doesn't represent a valid integer.
   *
   * @param s		The string to be converted.
   * @param multiplier	The multiplier for the suffixes "k", "m", "g", and "t".
   * 
   * @return		64-bit integer represented by the string.
   */
  uint64 byteStringToNumber(std::string const& s, uint32 multiplier = 1024);

  //============================================================================

  /**
   * Rounds up value to multiple of alignment value.
   */
  template <class T>
  inline T alignUp(T value, T alignment);

  /**
   * Rounds up value to multiple of alignment value.
   */
  template <class T>
  inline T alignDown(T value, T alignment);
	
  //============================================================================

  /**
   * Bind the current thread to a specific CPU.
   * 
   * Throws SystemException on error.
   * 
   * @param cpuId  The ID of the CPU to which the thread is to be bound.
   *               CPU IDs are numbered sequentially starting at 0.  The
   *               special value -1 will unbind the thread, allowing it
   *               to run on all CPUs.
   */
  void setAffinity(int32 cpuId);

  //============================================================================


  /**
   * Compile-time select. Taken from Loki.
   */
  template <bool flag, typename T, typename U>
  struct Select
  {
	typedef T Result;
  };

  template <typename T, typename U>
  struct Select<false, T, U>
  {
	typedef U Result;
  };

  template <bool flag, int32 T, int32 U>
  struct SelectInt
  {
	enum { Result = T };
  };

  template <int32 T, int32 U>
  struct SelectInt<false, T, U>
  {
	enum { Result = U };
  };

  //============================================================================

  /**
   * Constant representing infinite time.
   */
  extern boost::posix_time::time_duration const INFINITE;


  //============================================================================
  //============================================================================
  //
  // IMPLEMENTATION only below!!!
  //
  //============================================================================
  //============================================================================

  template <class T>
  inline T alignUp(T value, T alignment)
  {
	return((value + alignment - 1)/alignment)*alignment;
  }

  template <class T>
  inline T alignDown(T value, T alignment)
  {
	return(value / alignment) * alignment;
  }

  //============================================================================

  class ProgressBar
  /*
	If you want this to handle the window being resized, you'll need to provide a signal handler
	for SIGWINCH and pass the ws_row value to the progress bar.
  */
  {
  public:
	ProgressBar(std::string text = "", float percent = 0, bool showPercent = true)
	  :
	  percent(percent),
	  text(text),
	  showPercent(showPercent),
	  needsRefresh(true)
	{
	  struct winsize ws;
	  if (ioctl(0,TIOCGWINSZ,&ws)==0)
		{
		  this->columns = ws.ws_col;
		}
	  else
		{
		  this->columns = 80;
		}
	  this->refresh();
	}
	void windowResized(int columns)
	{
	  this->columns = columns;
	  this->needsRefresh = true;
	  this->refresh();
	}

	void updateText(std::string text)
	{
	  this->text = text;
	  this->needsRefresh = true;
	}

	void updatePercent(float percent)
	{
	  if (percent != this->percent)
		{
		  this->needsRefresh = true;
		  if (percent > 0 && percent <= 100)
			{
			  this->percent = percent;
			}
		  else if (percent > 100)
			{
			  this->percent = 100;
			}
		  else
			{
			  this->percent = 0;
			}
		}
	}

	void refresh()
	{
	  if (this->needsRefresh)
		{
		  printf("\033[2K["); //clear line, print opening bracket
		  uint barLength = (this->columns/3) + 1;
		  uint hashesNeeded = static_cast<int>(round((this->percent * barLength)/100));

		  uint length = 0;
		  for (;length < hashesNeeded; ++length)
			{
			  printf("#");
			}
		  for (;length < barLength; ++length)
			{
			  printf(" ");
			}
		  length += printf("] ");

		  if (this->showPercent)
			{
			  length += printf("%01.1f%% ", percent);
			}

		  printf("%s", this->text.substr(0, columns-length-1).c_str());
		  printf("\r");
		  fflush(stdout);
		  this->needsRefresh = false;
		}
	}

  private:
	uint columns;
	float percent;
	std::string text;
	bool showPercent;
	bool needsRefresh;
  };

} // end of namespace util
#endif //_UTIL_MISC_H
