/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

#include <exception>

#include <util/debug.h>
#include <util/string.h>
#include <util/exceptions.h>
#include <util/debug.h>
#include <string.h>

//============================================================================

namespace util 
{
	using namespace std;

	class ExceptionInitializator
	{
	public:
		ExceptionInitializator()
		{
			set_terminate(utilTerminate);
			set_unexpected(utilUnexpected);
		}

	private:
		static void utilTerminate()
		{
			printExceptionAndAbort("Terminated by an unhandled exception");
		}

		static void utilUnexpected()
		{
			printExceptionAndAbort("Terminated by an unexpected exception (thrown in a destructor?)");
		}

		static void printExceptionAndAbort(string info)
		{
			try
			{
				oss tmp;

				tmp << info << ": ";

				try
				{
					throw;
				}
				catch (Exception& e)
				{
					tmp << e;
				}
				catch (std::exception& e)
				{
					 tmp << e.what() << " (std library exception)";
				}
				catch (...)
				{
					tmp << "unknown";
				}

				tmp << "; BACKTRACE: " << backtrace() << endl;
				LOG(ERROR, tmp.str());
				cerr << tmp.str() << endl;
			}
			catch (...)
			{
			}

			::abort();
		}


	};


	static ExceptionInitializator whatever;

	//============================================================================
	
    Exception::Exception():
		name(typeid(*this).name())
	{
	}

	void Exception::initialize(string const& name,
							   string const& description,
							   Exception const* cause,
							   string const& filename,
							   int32 lineNo, 
							   string backtrace)
	{	

		this->name = name;
		this->description = description;
		this->filename = filename;
		this->lineNo = lineNo;
		this->backtrace = backtrace;

		if (cause)
		{
			this->causeMsgList = cause->causeMsgList;
			this->causeMsgList.push_front(cause->getFullMessage());
		}
	}
	
	void Exception::initialize(string const& name,
							   ostream const& ss,
							   Exception const* cause,
							   string const& filename,
							   int32 lineNo,
							   string backtrace)
	{
        return initialize(name, ostreamToString(ss), cause, filename, lineNo, backtrace);
	}
	
	Exception::~Exception()
	{
	}

	//============================================================================
	
	string Exception::getName() const
	{
		return this->name;
	}
	
	int32 Exception::getLineNo() const
	{
		return this->lineNo;
	}
	
	string Exception::getFilename() const
	{
		return this->filename;
	}
	
	string Exception::getDescription() const
	{
		return this->description;
	}
	
	string Exception::getMessage() const
	{
		string message = getName();
	
		if (!this->description.empty())
			message += " - " + getDescription();
	
		return message;
	}
	
	string Exception::getBacktrace() const
	{
		return this->backtrace;
	}

	//============================================================================
	
	string Exception::getFullMessage(bool withCauses) const
	{
		string message = getMessage();
	
		if (!this->filename.empty())
		{
			oss tmp;
			tmp << "" << this->lineNo;
			message += " <" + this->filename + ":" + tmp.str() + ">";
		}

		if (!this->backtrace.empty())
		{
			message += "\n\n------BACKTRACE------\n\n";
			message += this->backtrace + "\n---------------------\n";
		}
	
		if (withCauses)
		{
			CauseMsgList::const_iterator iter = this->causeMsgList.begin();
	
			while (iter != this->causeMsgList.end())
				message += "\n\t" + *iter++;
		}
	
		return message;
	}
	
	ostream& Exception::print(ostream& strm) const
	{
		strm << getFullMessage();
		return strm;
	}

	//============================================================================

    string sysError(string const& description, int32 error)
	{
		char buf[SAFE_STR_LEN] = { 0 };
		oss sstr;

		if (!description.empty())
		{
			sstr << description << "; ";
		}

		sstr << "system error #" << error;

		// the function below has non-conformant return type under GNU 
		char* msg = strerror_r(error, buf, sizeof(buf));

		if (msg[0] != 0)
		{
			sstr << " (" << msg << ")";
		}

		return sstr.str();
	}

	string sysError(ostream const& stream, int32 error)
	{
		return sysError(ostreamToString(stream), error);	 
	}

	ostream& OptException::print(ostream& strm) const
	{
		if (isSet())
		{
			this->exceptionCapsulePtr->print(strm);
		}
		else
		{
			strm << "Empty OptException";
		}

		return strm;
	}

	Exception const& OptException::getException() const
	{
		DASSERT(this->isSet(), "No exception set");
		return this->exceptionCapsulePtr->getException();
	}


} // end of namespace util

