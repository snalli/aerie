/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef HRFASTTIMETHREAD_H
#define HRFASTTIMETHREAD_H

/** \file hrFastTimeThread.cpp
 *  \brief threaded test/dev interface to HrFastTime
 */
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace util
{

    class TextrapThread : public boost::thread
    {
        public:
            TextrapThread( uint32_t _cpu, uint32_t _sleepTime=6U );
            virtual ~TextrapThread();
            virtual void start();
            virtual void join();
        protected:
            virtual void run();
            friend class HrFastTime;
        private:
            boost::shared_ptr<boost::thread> thrd;
            std::string name;
    };

}

#endif // HRFASTTIMETHREAD_H
