/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

#include <util/misc.h>
#include <util/string.h>
#include <util/gettid.h>
#include <util/boostTime.h>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <stdio.h>

using namespace std;
using namespace boost::posix_time;

namespace util 
{

	uint64 const KB = 1024;
	uint64 const MB = (1024 * KB);
	uint64 const GB = (1024 * MB);
	uint64 const TB = (1024 * GB);

	uint64 byteStringToNumber(string const& s, uint32 multiplier)
	{
		uint64 val = 0;
		uint64 dec = 0;
		uint64 decdiv = 1;
		uint64 mulf = 0;
		bool in_num = false;
		bool have_num = false;
		bool bvalid = false;
		bool have_dec = false;
		bool have_sign = false;
		bool is_neg = false;
		uchar c;
		uint64 kb;
		uint64 mb;
		uint64 gb;
		uint64 tb;

		if (multiplier == KB)
		{
			kb = KB;
			mb = MB;
			gb = GB;
			tb = TB;
		}
		else
		{
			kb = multiplier;
			mb = kb * kb;
			gb = kb * mb;
			tb = kb * gb;
		}

		string::const_iterator p = s.begin();
		while (p != s.end())
		{
			if (isspace(*p))
			{
				if (in_num)
				{
					in_num = false;
				}
				if (mulf != 0)
				{
					break;
				}
				bvalid = false;
			}
			else if (isdigit(*p))
			{
				if (in_num)
				{
					if (have_dec)
					{
						dec = (dec * 10) + (*p - '0');
						decdiv *= 10;
					}
					else
					{
						val = (val * 10) + (*p - '0');
					}
				}
				else if (!have_num)
				{
					have_num = true;
					val = *p - '0';
					in_num = true;
				}
				else
				{
					break;
				}
				bvalid = false;
			}
			else if (*p == '.')
			{
				if (have_dec)
				{
					in_num = false;
				}
				else
				{
					have_dec = true;
					in_num = true;
				}
			}
			else if (*p == '+' || *p == '-')
			{
				if (have_sign || have_num || have_dec)
				{
					break;
				}
				have_sign = true;
				if (*p == '-')
				{
					is_neg = true;
				}
			}
			else
			{
				in_num = false;
				if (have_dec && !have_num && dec == 0)
				{
					have_dec = false;
					break;
				}
				c = tolower(*p);
				if (c == 'k')
				{
					if (mulf == 0)
					{
						mulf = kb;
						bvalid = true;
					}
					else
					{
						mulf = 0;
					}
				}
				else if (c == 'm')
				{
					if (mulf == 0)
					{
						mulf = mb;
						bvalid = true;
					}
					else
					{
						mulf = 0;
					}
				}
				else if (c == 'g')
				{
					if (mulf == 0)
					{
						mulf = gb;
						bvalid = true;
					}
					else
					{
						mulf = 0;
					}
				}
				else if (c == 't')
				{
					if (mulf == 0)
					{
						mulf = tb;
						bvalid = true;
					}
					else
					{
						mulf = 0;
					}
				}
				else if (c == 'b')
				{
					if (bvalid)
					{
						bvalid = false;
					}
					else
					{
						mulf = 0;
					}
				}
				else
				{
					mulf = 0;
					THROW(ParseException, "Invalid string \"" << s << "\" doesn't specify number");
					break;
				}
			}
			++p;
		}
		if (!have_num && !have_dec)
		{
			THROW(ParseException, "Invalid string \"" << s << "\" doesn't specify number");
		}

		double dval = val;
		dval += static_cast<double>(dec) / decdiv;
		if (mulf != 0)
		{
			dval *= static_cast<double>(mulf);
		}
		val = static_cast<uint64>(dval);
		if (is_neg)
		{
			val *= -1;
		}
		return val;
	}
	
    static cpu_set_t affOrig;
	static pthread_once_t affInit = PTHREAD_ONCE_INIT;
	
    static void doAffInit(void)
	{
		CPU_ZERO(&affOrig);
		if (sched_getaffinity(getpid(), sizeof(affOrig), &affOrig) < 0)
		{
			LOG(ERROR, "sched_getaffinity failed: errno " << errno);
		}
	}

	void setAffinity(int32 cpuid)
	{
		cpu_set_t cs;
		pid_t tid;

		pthread_once(&affInit, doAffInit);

		tid = gettid();

		if (cpuid < 0)
		{
			if (cpuid != -1)
			{
				THROW_ERR1(SystemException, "Invalid CPU ID", EINVAL);
			}
			if (sched_setaffinity(tid, sizeof(affOrig), &affOrig) < 0)
			{
				LOG(ERROR, "sched_setaffinity failed: errno " << errno);
				THROW_ERR(SystemException, "sched_setaffinity failed");
			}
			else
			{
				LOG(INFO, "thread " << tid << " unbound from CPU "  << cpuid);
			}
		}
		else
		{
			if (CPU_ISSET(cpuid, &affOrig))
			{
				LOG(INFO, "restricting thread to run on cpu " << cpuid);
				CPU_ZERO(&cs);
				CPU_SET(cpuid, &cs);
				if (sched_setaffinity(tid, sizeof(cs), &cs) < 0)
				{
					LOG(ERROR, "sched_setaffinity failed: errno " << errno);
					THROW_ERR(SystemException, "sched_getaffinity failed");
				}
				else
				{
					LOG(INFO, "thread " << tid << " bound to CPU "  << cpuid);
				}
			}
			else
			{
				LOG(ERROR, "leaving affinity unchanged (requested cpuid " << cpuid << ")");
				THROW_ERR1(SystemException, "Invalid CPU ID", EINVAL);
			}
		}
	}


} // end of namespace util
