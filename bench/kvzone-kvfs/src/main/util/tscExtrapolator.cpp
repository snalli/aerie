/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

/** \file tscExtrapolator.cpp
 *  \brief A forward-correcting proportional-integral controller.
 */
#define TSCEX_MAIN // force some out-of-line inlines for lib
#include "tscExtrapolator.h"

#include "hrFastTime.h"
#include "tscSupport.h"

// debug:
//#ifdef HRVERBOSE
//#define HRTRACE( stuff ) do { std::cout<< stuff; std::cout.flush(); } while(0)
//#else
#define HRTRACE( stuff ) do{;}while(0)
//#endif

namespace util
{

    long double TscExtrapolator::sysSec( uint64_t tick )
    {
        long double ret;
        SeqReadLock srl( this->seqlock ); // see seqlock.h
        do
        {
            srl.reading();
            // Assumes tick > this->bTick
            ret = this->bSec + (tick - this->bTick) / this->bHertz; 
        }
        while( srl.retry() );
        return ret;
    }

    long double TscExtrapolator::progSec( uint64_t tick )
    {
        //return this->sysSec(tick) + this->cumJumpSec;
        long double ret;
        SeqReadLock srl( this->seqlock ); // see seqlock.h
        do
        {
            srl.reading();
            // Assumes tick > this->bTick
            ret = this->bSec + (tick - this->bTick) / this->bHertz
                + this->cumJumpSec;
        }
        while( srl.retry() );
        return ret;
    }

#if 0
    uint64_t TscExtrapolator::readTicks( struct timespec &systemTime )
    {
        // obsolete code XXX
        for(;;)
        {
            uint64_t k=util::rdtsc();
            clock_gettime( CLOCK_REALTIME, &systemTime );
            uint64_t k2=util::rdtsc();
            if( 1 ) // on same cpu, k2-k not unreasonably long
            {
                return (k>>1U) + (k2>>1U);
            }
        }
    }
#endif

    void TscExtrapolator::measure( util::TscMeasurement const &tscMeasurement,
           std::ostream *os /*=NULL*/ ) 
    {
        uint64_t cTick = tscMeasurement.tick;
        struct timespec const & c = tscMeasurement.time;
        long double cSec = util::tdouble(c);

        // Based on current predictor, what should systime cSec be?
        long double cSecPred = this->sysSec( cTick );
        long double cErr = cSecPred - cSec;

        if( os != NULL )
        {
            (*os)<<"\tb   = ( "<<bTick<<" , "<<util::tdouble(b)-HrFastTime::dsystime0<<" )"
                <<", c= b + "<<cTick-bTick<<" , "<<util::tdouble(c)-HrFastTime::dsystime0<<" )"
                <<"\n\tbSec= "<<bSec-HrFastTime::dsystime0
                <<" , bHertz="<<bHertz
                <<"\n\tcSec= "<<cSec-HrFastTime::dsystime0 
                <<" , cSecPred = "<<cSecPred-HrFastTime::dsystime0
                <<" , cErr = "<<cErr
                <<std::endl;
        }

        HRTRACE( /*"\n\ta = ("<<aTick<<','<<util::tdouble(a)-HrFastTime::dsystime0<<')'
                   <<*/"\n\tb = ("<<bTick<<','<<util::tdouble(b)-HrFastTime::dsystime0<<')'
                <<"\n\tc = ("<<cTick<<','<<util::tdouble(c)-HrFastTime::dsystime0<<')'
                <<"\n\tbSec="<<bSec-HrFastTime::dsystime0<<", bHertz="<<bHertz
                <<"\n\tcSec  = "<<cSec-HrFastTime::dsystime0 
                <<"\n\tcSecPred = "<<cSecPred-HrFastTime::dsystime0
                <<"\t\tcErr = "<<cErr
                <<std::endl
               );

        if(     cSec <= cSecPred - 0.5
                || cSec >= cSecPred + 0.5
                || cSec <= cSecPred - 0.5*(cSecPred-bSec)
                || cSec >= cSecPred + 0.5*(cSecPred-bSec)
          )
        {
            // XXX LOG time jump info !
            HRTRACE(" reacquire! "<<std::endl);
            // lacking info, we keep current rate, and our model update
            // looks pretty simple...

            util::ScopedSeqWriteLock sswl( this->seqlock );

            //this->a = this->a;           // no change, meaningless during time jump
            //this->aTick = this->aTick    // no change, irrelevant

            this->b = c;                   // raw measurement, but time jumped
            this->bTick = cTick;           // raw measurement

            this->bSec = cSec;             // time jump! (cSecPred would be continuous)
            //this->bHertz = this->bHertz; // w/ time jump, rate estimate untrustworthy.

            // We track the time jump, so that we can provide a cumulative
            // time jump value, or 'time since program start' function.
            this->cumJumpSec = cSecPred - cSec;
            return;
        }

        // What is the new linear interpolation startSeconds?
        // NOTE: This rate is based on actual raw measurements,
        // and not on derived parameters (bSec,bHertz).
        long double cHertz = (cTick - bTick) / tdiff(c,b);

        HRTRACE(" bHertz = "<<this->bHertz
                /*<<" = "<<(this->bTick-this->aTick) / (this->bSec - util::tdouble(a))*/
                <<"\ncHertz = "<<cHertz<<std::endl);

        // Our new predictor must be piecewise continuous.  So the new
        // predictor must go through the current prediction cSecPred, and
        // not the measured cSec.  So our new predictor can be inaccurate
        // at the measured point c.  ***BUT*** should adjust hertz so that
        // it becomes accurate after 'sleepTime' seconds. (self-correcting)
        // XXX TODO sleepTime should be measured from previous values XXX,
        //uint64_t dTick = (uint64_t)(cTick + HrFastTime::sleepTime*cHertz);

        // proportional control (lightly disguised by sleepTime)
        //long double dPredSec = cSec + (dTick - cTick) / cHertz;
        long double dPredSec = cSec + HrFastTime::sleepTime;
        //
        //Notice that:
        // "accurate after sleeptime" is essentially dictating the
        // correction rate of a proportional controller...
        //
        // So let's add a light integral control term.
        // (I think the rate is too jumpy for a derivative term,
        //  so I'll avoid a full P-I-D controller for now).
        // Sign of correction: cErr = cSecPred - cSec, so +ve cErr means
        // our prediction is generally too big.

        // The '0.5' is not critical.  This is just something to combat
        // long term drift, particularly since I was lazy and use
        // a nominal sleepTime that is sure to have systematic error!
        // This is a SMALL correction, to combat drifts of something
        // like 3 ms per hour (judging from fanCheck.cpp runs).
        dPredSec -= 0.5 * cErrSum;   // integral control term

        // dHertz, correct-after-sleeptime, is
        long double dHertz
            // = (dTick - cTick) / (dPredSec - cSecPred)
            = (HrFastTime::sleepTime*cHertz) / (dPredSec - cSecPred);
        HRTRACE(" dHertz = "<<dHertz<<std::endl);

        {
            util::ScopedSeqWriteLock sswl( this->seqlock );
            this->cErrSum += cErr;
            this->cErrSumSqr += cErr*cErr;

            // update model:
            //this->a = this->b;              // raw measurement
            //this->aTick = this->bTick;      // raw measurement

            this->b = c;                    // raw measurement
            this->bTick = cTick;            // raw measurement

            this->bSec = cSecPred;          // derived continuous time prediction
            this->bHertz = dHertz;          // derived error-correct-after-sleepTime tick rate
        }

        // They are different by around .01-.02 Hertz roundoff error,
        // which is totally negligible (can just use dHertz and not recalc)

        HRTRACE("cErrSum = "<<cErrSum<<", cErrSumSqr = "<<cErrSumSqr<<std::endl);
    }

} // util
