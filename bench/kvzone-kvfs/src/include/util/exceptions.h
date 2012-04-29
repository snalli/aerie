/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 *
 * Standard exception classes.
 */
#ifndef _UTIL_EXCEPTIONS_H
#define _UTIL_EXCEPTIONS_H

#include <errno.h>
#include <sstream>
#include <ostream>
#include <boost/shared_ptr.hpp>

#include "util/types.h"
#include "util/string.h"

namespace util
{
  std::string backtrace();

  //============================================================================

  /**
   * Standard base exception class. All exception classes should
   * derive from this one.
   *
   * DO NOT THROW Exception and it's derived classes directly
   * using throw clause -- use THROW macro instead.
   *
   * @see THROW
   * @see OptException
   */
  class Exception : public Printable
  {
  public:

	/**
	 * Constructs Exception object.
	 *
	 * initialize() is called by THROW or THROW_CAUSE macros to
	 * include various optional information about the exception.
	 *
	 * Users DO NOT have to call initialize() directly!
	 *
	 * Note, this class and derived classes should be created and
	 * via THROW or THROW_CAUSE macros only.
	 *
	 * @see initialize
	 */
	Exception();

	/**
	 * The destructor.
	 */
	virtual ~Exception();

	/**
	 * Get exception class name.
	 *
	 * @return
	 */
	virtual std::string getName() const;

	/**
	 * Return formatted message consisting of: exception
	 * class name, description and (optional) error
	 * string.
	 *
	 * @return The formatted message.
	 */
	virtual std::string getMessage() const;

	/**
	 * Get fully formatted error message consisting of: exception
	 * class name, description, (optional) error string,
	 * filename and line information.
	 *
	 * If withCauses flag is true the messages for nested
	 * exception, i.e. causes will be appended, each on new
	 * line to the resulting message.
	 *
	 * @param withCauses Adds causes messages if true.
	 *
	 * @return The formatted message.
	 */

	virtual std::string getFullMessage(bool withCauses = true) const;

	/**
	 * Return line number.
	 *
	 */
	int32 getLineNo() const;

	/**
	 * Return the source filename the exception has
	 * been thrown at.
	 *
	 */
	std::string getFilename() const;

	/**
	 * Return the description string.
	 */
	std::string getDescription() const;

	/**
	 * Returns backtrace.
	 */
	std::string getBacktrace() const;

	/**
	 * Initializes Exception object.
	 *
	 * Users do NOT have to call initialize() directly!
	 *
	 * @param name
	 * @param description
	 * @param cause
	 * @param filename
	 * @param lineNo
	 * @param backtrace
	 *
	 * @see THROW
	 */
	virtual void initialize(std::string const& name,
							std::string const& description = "",
							Exception const* cause = NULL,
							std::string const& filename = "",
							int32 lineNo = -1,
							std::string backtrace = "");

	void initialize(std::string const& name,
					std::ostream const& ss,
					Exception const* cause = NULL,
					std::string const& filename = "",
					int32 lineNo = -1,
					std::string backtrace = "");

	virtual std::ostream& print(std::ostream& strm) const;

  private:
	std::string name;
	std::string description;
	std::string filename;
	int32 lineNo;
	std::string backtrace;

	typedef std::list<std::string> CauseMsgList;
	CauseMsgList causeMsgList;
  };

  //============================================================================

  /**
   * Envelope class for Exception classes. 
   * This class is similar to ExceptionHolder, except
   * it does not require to contain an Exception.
   * 
   * Generally ExceptionHolder is preferable over
   * OptException. 
   */
  class OptException : public Printable
  {
  public:
	/**
	 * Sets the exception. Use EXCEPTION macro to create argument
	 * for this function. This will allow to include file/line
	 * information.
	 *
	 * Example:
	 *
	 * OptException oe;
	 * oe.set(EXCEPTION(IoException, "something bad happend"));
	 *
	 * @param TExc
	 * @param exc
	 * @see EXCEPTION
	 */
	template <typename TExc>
	void set(TExc const& exc);

	/**
	 * Returns true if an exception has been set or false
	 * otherwise.
	 */
	bool isSet() const;

	/**
	 * Throws the underlying exception if present, otherwise
	 * no-op.
	 */
	void throwIfAny() const;

	/**
	 * Clears the underlying exception.
	 */
	void clear();

	/**
	 * Prints the underlying exception on an ostream.
	 */
	virtual std::ostream& print(std::ostream& strm) const;

	/**
	 * Returns reference to contained Exception object.
	 * 
	 * Fails with an assertion if does not have exception.
	 */
	Exception const& getException() const;

  private:
	class ExceptionCapsule;
	template <typename TExc> class ExceptionCapsuleImpl;

	boost::shared_ptr<ExceptionCapsule> exceptionCapsulePtr;
  };

  //============================================================================

  /**
   * Envelope class for Exception classes. 
   * 
   * It is used to pass exception between thread boundaries,
   * e.g. in callback functions with retaining the actual 
   * exception type.
   */
  class ExceptionHolder : public Printable
  {
  public:
	/**
	 * Creates ExceptionHolder with an Exception instance.
	 * 
	 * Preferably use EXCEPTION macro to create instances 
	 * of ExceptionHolder. Example:
	 *
	 * makeExceptionHolder(EXCEPTION(IoException, 
	 *                     "something bad happend"));
	 * 
	 * Note, if ExceptionHolder is created in a catch
	 * handler the exception type may not be retained -
	 * the type will be equivalent to the type in the
	 * catch handler. However, the description and
	 * the name of the original type will be retained.
	 * Example:
	 * 
	 * try
	 * {
	 * 		THROW(IoExcecption, "problem");
	 * }
	 * catch (Exception& e)
	 * {
	 * 		// ExceptionHolder will contain Exception,
	 * 		// not IoException
	 * 		ExceptionHolder eh(e);
	 * 
	 * 		try
	 * 		{
	 * 			eh.throwIt();
	 * 		}
	 * 		catch (IoException& e)
	 * 		{
	 * 			// we'll not get here
	 * 		}
	 * 		catch (Exception& e)
	 * 		{
	 * 			// but here
	 * 		}
	 * }
	 * 
	 *
	 * @param TExc
	 * @param exc
	 * @see EXCEPTION
	 */
	template <typename TExc>
	ExceptionHolder(TExc const& exc);


	/**
	 * Creates instance initializing it from an OptException.
	 */
	ExceptionHolder(OptException const& optExc);

	/**
	 * Throws the underlying exception.
	 */
	void throwIt() const;

	/**
	 * Prints the underlying exception on an ostream.
	 */
	virtual std::ostream& print(std::ostream& strm) const;

	/**
	 * Returns reference to contained Exception object.
	 */
	Exception const& getException() const;

	/**
	 * Returns reference to contained OptException
	 */
	OptException getOptException() const;

  private:
	/**
	 * Not available.
	 */
	ExceptionHolder& operator=(ExceptionHolder const&);

	OptException optExc;
  };

  //============================================================================
  /**
   * Throw new exception. The cl paramater specifies the
   * exception class, the desc is the description,
   * i.e. context-specific message.
   *
   * Note, that type of the exception should be implied by
   * the class name, not the description. For example,
   * don't throw same exception class to report memory
   * allocation problem and I/O error. A good of a description
   * for e.g. IoException would be:
   * "read() failed when reading from file foo.txt".
   *
   * Use oss stream (ostringstream) to produce meaningful
   * exceptions descriptions. E.g:
   *
   * string filename;
   * ...
   * THROW(IoException, "read() failed when reading from " << filename);
   *
   * @see Exception
   * @see THROW_ERR
   * @see THROW_CAUSE
   * @see oss
   * @see ostringstream
   */
#define THROW(cl, desc)													\
  {																		\
	cl ex;																\
	util::oss tmp__;													\
	tmp__ << desc;														\
	ex.initialize(std::string(MAKE_STR(cl)), tmp__.str(), NULL, __FILE__, __LINE__, util::backtrace()); \
	throw ex;															\
  }

  /**
   * Inline friendly variant of THROW.
   * 
   * Regular macros affect inline optimization on gcc (4.0.2). Stream formatting (eg. "foo" << 123)
   * negatively impacts the compiler to inline.* "Inline friendly" macros delegate the stream
   * formating to a separate function (static in a local class), which effectively eliminates the 
   * problem.
   * 
   * Unlike the variant without underscore, it does not take stream like formating expression
   * (e.g. "foo " << 123). Instead, takes variable number of arguments that internally are 
   * concatenated, using stream formatting - everything passed must be supported by operator<<.
   * 
   * For example:
   * 
   * int32 i;
   * MACRO(arg, "foo " << 123 << " bar" << i);
   * 
   *  would be written as
   *  
   * int32 i;
   * _MACRO(arg, "foo ", 123, " bar", _VAR(i));
   * 
   * are equivalent, however the latter case will yield code should be optimizable better by
   * gcc.
   * 
   * Note, local variables need to be enclosed in _VAR macro.  Global or static variables
   * or constants can appear "as typed", and do not need the _VAR macro.  If converting
   * code and you forget _VAR, then you will get a compiler error
   *
   * "error: use of parameter from containing function"
   *
   * that points out the variable you should enclose in _VAR.
   *
   * More complex things, like evaluated expressions are also supported, via
   * other tagging mechanisms:  see tricks.h macros LX (local expression) and
   * IX (internal expression).
   *
   * As an example of logging an evaluated expression, you might desire to write:
   * \verbatim
   * _DLOG(2, " Session access mode is "
   *        , LX( char const *sMode=( session.getSessionMode()==hv2::READ_ONLY?
   *                                  "R/O" : "R/W" ) )
   *        , _VAR(sMode)
   *        , ", numeric value "
   *      )
   * \endverbatim
   * It does not matter very much where the LX `local expression' is placed, since
   * the macro strips all LX and IX things before starting to generate code. I.e.,
   * the LX could equally well have been anywhere after the `_DLOG(2,' in the above
   * example.  LX is also useful to define a local variable holding some return
   * value, that is passed via _VAR.
   * 
   * A gcc ICE that is still around may occur if you use \c _VAR(this->xxx) where this
   * is a template function.  This ICE happens when using 'typeof' within templates
   * sometimes.  You can work around it by explicitly supplying the type of this->xxx
   * within an \c "LX( Xxxtype const &thisXxx = this->xxx ), _VAR(thisXxx)" replacement
   * (Ugly, because you have to look up and type the actual type of this->xxx).
   *
   * In some cases, it may not be possible to work around this g++ bug (yet --
   * language standardization may supply `auto' or `typeof' or `decltype'
   * officially someday --- just use the inlined macros until then!)
   *
   */
#define _THROW(cl, ...) UTIL_INLINE_FRIENDLY(THROW, 1, 1, cl, __VA_ARGS__ )

  /**
   * Throws an exception. Includes the current error
   * information (errno variable) in the exception description.
   *
   * @see THROW
   * @see THROW_ERR1
   */
#define THROW_ERR(cl, desc)												\
  {																		\
	cl ex;																\
	util::oss tmp__;													\
	tmp__ << desc;														\
	ex.initialize(std::string(MAKE_STR(cl)), util::sysError(tmp__.str()), NULL, __FILE__, __LINE__, util::backtrace()); \
	throw ex;															\
  }

  /**
   * Inline friendly variant of THROW_ERR.
   * 
   * @see _THROW
   */
#define _THROW_ERR(cl, ...) UTIL_INLINE_FRIENDLY(THROW_ERR, 1, 1, cl, __VA_ARGS__ )

  /**
   * Similiar to THROW_ERR, but takes the error value as
   * an extra argument as opposed to taking it from errno
   * variable.
   *
   * @see THROW
   * @see THROW_ERR
   */
#define THROW_ERR1(cl, desc, err)										\
  {																		\
	cl ex;																\
	util::oss tmp__;													\
	tmp__ << desc;														\
	ex.initialize(std::string(MAKE_STR(cl)), sysError(tmp__.str(), (err)), NULL, __FILE__, __LINE__, util::backtrace()); \
	throw ex;															\
  }

  /**
   * Inline friendly variant of THROW_ERR1.
   * 
   * @see _THROW
   */
#define _THROW_ERR1(cl, err, ...) UTIL_INLINE_FRIENDLY(THROW_ERR1, 2, 1, cl, CT(err), __VA_ARGS__ )

  /**
   * Throws exception with a "cause". This is useful if
   * an exception is being thrown as a result of an exception
   * caught from an inner layer and the inner layer's exception
   * should not be exposed.
   *
   * The original exception (cause) description and information
   * is retained in the new exception.
   *
   * @see THROW
   */
#define THROW_CAUSE(cl, desc, cause)									\
  {																		\
	cl ex;																\
	util::oss tmp__;													\
	tmp__ << desc;														\
	ex.initialize(std::string(MAKE_STR(cl)), tmp__.str(), (&cause), __FILE__, __LINE__, util::backtrace()); \
	throw ex;															\
  }

  /**
   * Inline friendly variant of THROW_CAUSE.
   * 
   * @see _THROW
   */
#define _THROW_CAUSE(cl, cause, ...) UTIL_INLINE_FRIENDLY(THROW_CAUSE, 2, 1, cl, CT(cause), __VA_ARGS__ )

  /**
   * Returns an initialized exception (with class, description
   * and file/line information). This is useful in conjunction
   * with OptException::set function.
   *
   * @see OptException
   * @see THROW
   */
#define EXCEPTION(cl, desc) makeException<cl>(MAKE_STR(cl), util::ostreamToString(util::oss() << "" << desc), __FILE__, __LINE__, util::backtrace())

  /**
   * THROW_ERR equivalent for EXCEPTION.
   */
#define EXCEPTION_ERR(cl, desc) makeException<cl>(MAKE_STR(cl), sysError(util::oss() << desc), __FILE__, __LINE__, util::backtrace())

  /**
   * THROW_ERR1 equivalent for EXCEPTION.
   */
#define EXCEPTION_ERR1(cl, desc, err) makeException<cl>(MAKE_STR(cl), sysError((oss() << desc), (err)), _FILE__, __LINE__, util::backtrace())

  /**
   * THROW_CAUSE equivalent for EXCEPTION.
   */
#define EXCEPTION_CAUSE(cl, desc, cause) makeException<cl>(MAKE_STR(cl), sysError(util::oss() << desc), __FILE__, __LINE__, util::backtrace(), (&cause))

  //============================================================================

  /**
   * A non-IO error returned by the operating system.
   */
  class SystemException : public Exception
  {
  };

  /**
   * An error related to an I/O operation. All I/O related exception types should derive from this type.
   */
  class IoException : public Exception
  {
  };

  /**
   * Indicates end-of-stream or end-of-file condition.
   */
  class EofException : public IoException
  {
  };

  /**
   * Indicate device out of space condition.
   */
  class OutOfSpaceException : public IoException
  {
  };

  /**
   * A memory allocation failed.
   */
  class NotEnoughMemoryException : public Exception
  {
  };

  /**
   * An internal, unrecoverable exception.
   */
  class InternalException : public Exception
  {
  };

  /**
   * An exception related to parsing.
   */
  class ParseException : public Exception
  {
  };

  /**
   * Attempt to open non-existing file or device.
   */
  class FileDoesNotExistException : public IoException
  {
  };

  // typo, for backward copatibility
  typedef FileDoesNotExistException FileDoesNotExistsException;

  /**
   * Attempt to create and exclusively open a file.
   */
  class FileAlreadyExistException : public IoException
  {
  };

  /**
   * Error while rebuilding data (for example data veryfication fails)
   */
  class DataException : public IoException
  {
  };

  /**
   * Indicates that a lookup on a container fails.
   * Use judicously. In many cases an assertion is more
   * appropriate.
   */
  class NoSuchElementException : public Exception
  {
  };

  /**
   * Indicates that an element already exists in a
   * container.
   * Use judicously. In many cases an assertion is more
   * appropriate.
   */
  class ElementAlreadyExistsException : public Exception
  {
  };

  /**
   * Indicates a logical error in an application. Intended use is
   * for errors that are communicated to the human user thru 
   * console or GUI.
   */
  class Error : public Exception
  {
  };

  /**
   * Indicates that a data format is not supported.
   */
  class UnsupportedDataFormatException : public Exception
  {
  };

  /**
   * Indicates that a data format is not support.
   */
  class UnsupportedDataFormatVersionException : public Exception
  {
  };

  /**
   * Indicates corrupted data.
   */
  class DataCorruptionException : public Exception
  {
  };

  //============================================================================
  //
  // IMPLEMENTATION below
  //
  //============================================================================

  // INTERNAL - do not use!
  // errno can be a macro (is in glibc)
  std::string sysError(std::string const& description, int32 error = errno);

  // INTERNAL - do not use!
  // errno can be a macro (is in glibc)
  std::string sysError(std::ostream const& stream, int32 error = errno);

  // INTERNAL - do not use!
  template <typename TExc>
  TExc makeException(std::string className, std::string desc, std::string file, uint32 lineNo, std::string backtrace,
					 Exception const* cause = NULL)
  {
	TExc ex;
	ex.initialize(className, desc, cause, file, lineNo, backtrace);
	return ex;
  }

  //============================================================================
  // OptException
  //============================================================================

  class OptException::ExceptionCapsule : public Printable
  {
  public:
	virtual void throwException() const = 0;

	virtual ~ExceptionCapsule() {};

	virtual Exception const& getException() const = 0;
  };

  template <typename TExc>
  class OptException::ExceptionCapsuleImpl : public ExceptionCapsule
  {
  public:
	ExceptionCapsuleImpl(TExc const& exc):
	  exc(exc)
	{
	}

	virtual void throwException() const
	{
	  throw this->exc;
	}

	virtual std::ostream& print(std::ostream& strm) const
	{
	  return this->exc.print(strm);
	}

	Exception const& getException() const
	{
	  return this->exc;
	}

	TExc const exc;
  };

  template <typename TExc>
  inline void OptException::set(TExc const& exc)
  {
	this->exceptionCapsulePtr.reset(new ExceptionCapsuleImpl<TExc>(exc));
  }

  inline bool OptException::isSet() const
  {
	return this->exceptionCapsulePtr;
  }

  inline void OptException::clear()
  {
	this->exceptionCapsulePtr.reset();
  }

  inline void OptException::throwIfAny() const
  {
	if (this->exceptionCapsulePtr)
	  {
		try
		  {
			this->exceptionCapsulePtr->throwException();
		  }
		catch (...)
		  {
			throw;
		  }
	  }
  }

  //============================================================================
  // ExceptionHolder
  //============================================================================

  template <typename TExc>
  inline ExceptionHolder::ExceptionHolder(TExc const& exc)
  {
	this->optExc.set(exc);
  }

  inline ExceptionHolder::ExceptionHolder(OptException const& optExc):
	optExc(optExc)
  {
  }

  inline void ExceptionHolder::throwIt() const
  {
	this->optExc.throwIfAny();
  }

  inline std::ostream& ExceptionHolder::print(std::ostream& strm) const
  {
	return this->optExc.print(strm);
  }

  inline Exception const& ExceptionHolder::getException() const
  {
	return this->optExc.getException();
  }
	
  inline OptException ExceptionHolder::getOptException() const
  {
	return this->optExc;
  }

  //============================================================================

  /**
   * @deprecated Use DASSERT/PASSERT instead!
   */
  class RuntimeException : public Exception
  {
  };

  /**
   * @deprecated Use DASSERT/PASSERT instead!
   */
  class NotImplementedException : public RuntimeException
  {
  };

  /**
   * @deprecated Use DASSERT/PASSERT instead!
   */
  class IllegalArgumentException : public RuntimeException
  {
  };

  /**
   * @deprecated Use DASSERT/PASSERT instead!
   */
  class IllegalStateException : public RuntimeException
  {
  };

  /**
   * @deprecated Use DASSERT/PASSERT instead!
   */
  class IndexOutOfBoundsException : public RuntimeException
  {
  };


} // end of namespace util

#endif // _UTIL_EXCEPTIONS_H
