/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include <iostream>
#include <cstdlib>
#include <queue>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include <boost/random/lagged_fibonacci.hpp>
#include <tr1/unordered_set>

#include "util/misc.h"
#include "util/completion.h"
#include "keyValueStore/keyValueStoreTypes.h"
#include "keyValueStore/keyValueStore.h"
#include "util/boostTime.h"
#include </scratch/nvm/stamnos/libfs/src/kvfs/client/c_api.h>

using namespace std;
using namespace boost::posix_time;

using namespace util;
using namespace keyValueStore;

uint32 const windowSize = 100;

template <typename T>
class MovingAverageCalculator
{
    public:
        MovingAverageCalculator(uint32 windowSize)
            : windowSize(windowSize)
        {
        }

        void addNext(T const& t)
        {
            if (this->currWindow.size() >= windowSize)
            {
                this->currWindow.erase(this->currWindow.begin());
            }
            DLOG(DINFO, " MovingAverageCalculator got latency " << t);
            this->currWindow.push_back(t);
        }

        T getAverage()
        {
            T sum = 0;
            Iterator i = this->currWindow.begin();
            for (; i != this->currWindow.end(); ++i)
            {
                sum += *i;
            }
            T avg = sum / this->currWindow.size();
            DLOG(DINFO, " MovingAverageCalculator returning average " << avg);
            return (avg);
        }
    private:
        uint32 windowSize;
        typedef vector<T> MACVector;
        typedef typename MACVector::iterator Iterator;
        MACVector currWindow;
};

class TargetIOPSGenerator
{
    public:
        TargetIOPSGenerator(uint64 startIops, double targetLatency)
            : currIops(startIops),
            targetLatency(targetLatency),
            lastIops(1000),
            lastLatency(0),
            lastDiff(0),
            rampup(true)
    {
        DLOG(DINFO, "TargetIOPSGenerator targetLatency " << this->targetLatency << " starting IOPS " << this->currIops);
    }
        bool needsAdjustment(double currAvgLatency) const
        {
            if (isLatencyWithinBounds(currAvgLatency))
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        uint64 adjustTargetIops(double currAvgLatency, double avgLatency, uint64 hitTargetIops)
        {
            DLOG(DINFO, "adjustTargetIops targetLatency " << this->targetLatency << " current latency " << currAvgLatency);
            if (this->rampup == true)
            {
                if (currAvgLatency < targetLatency)
                {
                    this->currIops = (this->currIops * 110)/100;
                    DLOG(DINFO, "TargetIOPSGenerator ramping up, new target IOPS " << this->currIops);
                }
                else
                {
                    this->rampup = false;
                    this->lastIops=this->currIops;
                    this->currIops = this->lastIops/2;
                    this->curDivFactor = 2;
                    this->up = false;
                    DLOG(DINFO, "TargetIOPSGenerator ramp up is done new target IOPS " << this->currIops);
                }
            }
            else //not in rampup mode, normal operation
            {
                if (currAvgLatency > targetLatency && avgLatency > targetLatency)
                {
                    this->up = false;
                    this->lastIops = this->currIops;
                    if ((avgLatency * 1.25) > targetLatency)
                    {
                        this->currIops /= 2;
                        this->curDivFactor = 2;
                    }
                    else
                    {
                        this->currIops -= this->currIops/(this->curDivFactor * (hitTargetIops+1));
                        if (this->curDivFactor > 2 && this->over) this->curDivFactor /= 2;
                    }
                    this->over = true;
                    DLOG(DINFO, "TargetIOPSGenerator resetting target IOPS to " << this->currIops);
                }
                else if (currAvgLatency < targetLatency)
                {
                    this->over = false;
                    if (!this->up)
                    {
                        if (this->lastIops/this->curDivFactor < 1 && this->curDivFactor > 2)
                        {
                            this->up = true;
                            this->curDivFactor /= 2;
                        }
                        else
                        {
                            this->curDivFactor *= 2;
                        }
                    }
                    else if (this->curDivFactor > 2)
                    {
                        this->curDivFactor /= 2;
                    }
                    else
                    {
                        this->up = false;
                        this->curDivFactor *= 2;
                    }
                    this->currIops += (this->lastIops/(this->curDivFactor * (hitTargetIops+1)));
                    DLOG(DINFO, "TargetIOPSGenerator increasing target IOPS to " << this->currIops);
                }
                else
                {
                    this->curDivFactor *= 2;
                }
            }

            return this->currIops;
        }

    private:
        uint64 currIops;
        double targetLatency;
        uint64 lastIops;
        double lastLatency;
        double lastDiff;
        uint64 curDivFactor;
        bool rampup;
        bool up;
        bool over;

        bool isLatencyWithinBounds(double currLatency) const
        {
            double min = targetLatency - 0.025 * targetLatency;
            double max = targetLatency + 0.025 * targetLatency;

            if (min <= currLatency && currLatency <= max)
            {
                return true;
            }

            return false;
        }
};

uint64 const startIops = 1000;
uint64 const maxChangeIopsAfter = 5000;
uint64 changeIopsAfter = startIops/2;
TargetIOPSGenerator* iopsGenerator;
int errorCode = 0;
uint64 const KB = 1024;
uint64 startTime = 0;
uint64 endTime = 0;
uint64 totalRequests = 100000;
double globalLatency = 0;
unsigned int minBlockSize = 32768;
unsigned maxBlockSize = 262144;
uint64 generatorIops = 1000;
bool iopsMode = false;
double targetLatency = 1000;
bool latencyMode = false;
volatile bool noMoreRequests = false;
volatile bool waitingForEmpty = false;

MovingAverageCalculator<double> avgCalculator(windowSize);

unsigned int seed = 777;
unsigned int avgLifeTime = 5;
unsigned int deviation = 1;
KeyValueStoreType kvType = BERKELEYDB;
uint16_t numThreads = 10;
string logFilePath = "";
uint64 storeSize=1024 * 1024 * 1024; //1GB
uint64 preWriteSize = 0;
uint64 maximumKVSLoad = 0;

int readPct = 75;

uint64 latencylt50m = 0;
uint64 latencylt100m = 0;
uint64 latencylt250m = 0;
uint64 latencylt500m = 0;
uint64 latencylt750m = 0;
uint64 latencylt1000m = 0;
uint64 latencylt2 = 0;
uint64 latencylt4 = 0;
uint64 latencylt10 = 0;
uint64 latencylt20 = 0;
uint64 latencylt50 = 0;
uint64 latencylt100 = 0;
uint64 latencylt250 = 0;
uint64 latencylt500 = 0;
uint64 latencylt750 = 0;
uint64 latencylt1000 = 0;
uint64 latencylt2000 = 0;
uint64 latencygte2000 = 0;

double totalWriteLatency = 0;
double totalWriteLatencySqr = 0;
double totalReadLatency = 0;
double totalReadLatencySqr = 0;
double totalDeleteLatency = 0;
double totalDeleteLatencySqr = 0;

uint64 totalWritten = 0;
uint64 totalRead = 0;
uint64 totalDeleted = 0;

uint64 pendingReqs = 0;
uint64 pendingWrites = 0;
uint64 pendingReads = 0;
uint64 pendingDeletes = 0;

#define NUM_RAND_VALUES 1000000
unsigned int blockSizes[NUM_RAND_VALUES];
uint64 currBlockSizeIndex = 0;

uint64 currKey = 0x09;

boost::mutex submissionMutex;
uint64 numOutstanding = 0;
uint64 numWrites = 0;
uint64 totalNumWrites = 0;
uint64 totalNumLookups = 0;
uint64 numLookups = 0;
bool writeFinished = false;
bool readFinished = false;
bool delFinished = false;

Completion waitCond;

bool prewritesIssued = false;
uint64 numPrewrites = 0;
util::Completion prewriteCond;

boost::mutex writeMut;
boost::condition writeCond;

boost::mutex readMut;
boost::condition readCond;

boost::mutex delMut;
boost::condition delCond;

char* buffer;

unsigned int currSubmitReqSize = 0;
boost::mutex blockMutex;
bool blocked = false;
Completion blockCond;

uint64 totalCycles = 0;
uint64 writeSubmissionLatency = 0;

void gen_random(char *s, const int len) 
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    strcpy(s, "/pxfs/");
    for (int i = 6; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

void generateBlockSizes()
{
    srand(seed);

    unsigned int range = (maxBlockSize - minBlockSize) + 1;

    for (uint64 i = 0; i < NUM_RAND_VALUES; ++i)
    {
        blockSizes[i] = minBlockSize + static_cast<unsigned int>(range * 1.0 * rand() / (RAND_MAX * 1.0));
    }
}

void updateLatencyBuckets(double reqLatency)
{
    if (reqLatency <= 50)
    {
        ++latencylt50m;
    }
    else if (reqLatency <= 100)
    {
        ++latencylt100m;
    }
    else if (reqLatency <= 250)
    {
        ++latencylt250m;
    }
    else if (reqLatency <= 500)
    {
        ++latencylt500m;
    }
    else if (reqLatency <= 750)
    {
        ++latencylt750m;
    }
    else if (reqLatency <= 1000)
    {
        ++latencylt1000m;
    }
    else if (reqLatency <= 2 * 1000)
    {
        ++latencylt2;
    }
    else if (reqLatency <= 4 * 1000)
    {
        ++latencylt4;
    }
    else if (reqLatency <= 10 * 1000)
    {
        ++latencylt10;
    }
    else if (reqLatency <= 20 * 1000)
    {
        ++latencylt20;
    }
    else if (reqLatency <= 50 * 1000)
    {
        ++latencylt50;
    }
    else if (reqLatency <= 100 * 1000)
    {
        ++latencylt100;
    }
    else if (reqLatency <= 250 * 1000)
    {
        ++latencylt250;
    }
    else if (reqLatency <= 500 * 2000)
    {
        ++latencylt500;
    }
    else if (reqLatency <= 750 * 1000)
    {
        ++latencylt750;
    }
    else if (reqLatency <= 1000 * 1000)
    {
        ++latencylt1000;
    }
    else if (reqLatency <= 2000 * 1000)
    {
        ++latencylt2000;
    }
    else if (reqLatency > 2000 * 1000)
    {
        ++latencygte2000;
    }
}

void printStats()
{
    uint64 totalReqs = totalWritten + totalRead + totalDeleted;
    double totalLatency = double((endTime -  startTime) * 1.0) / 1000;

    cout << endl << "total IO Requests " << totalReqs << " total time(s) " << totalLatency << " IOPS " << double (totalReqs) / totalLatency << endl;
    cout << "avg latency " << ((globalLatency/totalReqs)/1000) << endl;

    if (totalWritten != 0)
    {
        double avgWriteLatencyMicrosec = (totalWriteLatency * 1.0) / totalWritten;
        double avgWriteLatency = avgWriteLatencyMicrosec / 1000; 
        double writeDev = (sqrt((totalWriteLatencySqr / totalWritten) - (avgWriteLatency * avgWriteLatency))) / 1000;
        cout << "Writes numRequests " << totalWritten << " total latency(ms) " << fixed << setprecision(2) << (totalWriteLatency * 1.0 /1000) << " avg latency(ms) " << fixed << setprecision(2) << avgWriteLatency << " std deviation " << fixed << setprecision(2) << writeDev << endl; 
    }
    if (totalRead != 0)
    {
        double avgReadLatencyMicrosec = (totalReadLatency * 1.0) / totalRead;
        double avgReadLatency = avgReadLatencyMicrosec / 1000; 
        double readDev = (sqrt((totalReadLatencySqr / totalRead) - (avgReadLatency * avgReadLatency))) / 1000;
        cout << "Reads numRequests " << totalRead << " total latency(ms) " << fixed << setprecision(2) << (totalReadLatency * 1.0/1000) << " avg latency(ms) " << fixed << setprecision(2) << avgReadLatency << " std deviation " << fixed << setprecision(2) << readDev << endl; 
    }
    if (totalDeleted != 0)
    {
        double avgDelLatencyMicrosec = (totalDeleteLatency * 1.0) / totalDeleted;
        double avgDelLatency = avgDelLatencyMicrosec / 1000; 
        double delDev = (sqrt((totalDeleteLatencySqr / totalDeleted) - (avgDelLatency * avgDelLatency))) / 1000;
        cout << "Deletes numRequests " << totalDeleted << " total latency(ms) " << fixed << setprecision(2) << (totalDeleteLatency * 1.0/1000) << " avg latency(ms) " << fixed << setprecision(2) << avgDelLatency << " std deviation " << fixed << setprecision(2) << delDev << endl; 
    }

    cout << " lat (usec): 50=" << latencylt50m << ", 100=" << latencylt100m << ", 250=" << latencylt250m << ", 500=" << latencylt500m << ", 750=" << latencylt750m << endl;
    cout << " lat (usec): 1000=" << latencylt1000m << endl;
    cout << " lat (msec): 2=" << latencylt2 << ", 4=" << latencylt4 << ", 10=" << latencylt10 << ", 20=" << latencylt20 << ", 50=" << latencylt50 << endl;
    cout << " lat (msec): 100=" << latencylt100 << ", 250=" << latencylt250 << ", 500=" << latencylt500 << ", 750=" << latencylt750 << ", 1000=" << latencylt1000 << endl;
    cout << " lat (msec): 2000=" << latencylt2000 << ", >=2000=" << latencygte2000 << endl;
}

void usage(string name)
{
    cout <<"Usage: " << name << " [options]:" << endl;
    cout <<" -p path                    filesystem path to kvstore" << endl;
    cout <<" -z storeSize               expected key-value store size" << endl;
    cout <<" -P prewrite factor         prewrite data (percent of store size)" << endl; 
    cout <<" -s numRequests             total IO requests to generate" << endl;
    cout <<" -m minBlockSize            minimum block size" << endl;
    cout <<" -x maxBlockSize            maximum block size" << endl;
    cout <<" -o targetIops              target iops for data generator (note: must use either -o or -a)" << endl;
    cout <<" -a targetLatency           target latency in us (note: must use either -o or -a)" << endl; 
    cout <<" -L read percent            percentage of read requests to generate" << endl;
    cout <<" -t keyValueStoreType       type of key-value store (bdb, tc, mem, sqlite or noop)" << endl;
    cout <<" -r threads                 number of threads for key value store" << endl;
    cout <<" -h                         show help info" << endl;
}

class MyKeyType 
{
    public:
        explicit MyKeyType(uint64 key)
            : key(key)
        {
	    gen_random(skey, 16);
        }

        explicit MyKeyType()
            : key(0)
        {
	    gen_random(skey, 16);
        }

        MyKeyType(MyKeyType const& other)
        {
            this->key = other.key;
	    gen_random(skey, 16);
        }

        ~MyKeyType()
        {
        }

        uint64 getKey() const
        {
            return this->key;
        }

	char* getsKey()
	{
	   return this->skey;
	}

        virtual bool operator<(MyKeyType const& other) const
        {
            return (this->key < other.key); 
        }

        bool operator==(MyKeyType const& other) const
        {
            return (this->key == other.key);
        }

        uint32 getSerializedSize() const
        {
            return (sizeof(this->key));
        }

        void serialize(char* buffer) const
        {
            //memcpy(buffer, &this->key, sizeof(this->key));
	    strcpy(buffer, skey);
        }

        void deserialize(char const* buffer)
        {
            memcpy(&this->key, buffer, sizeof(this->key));
        }

    private:
        uint64 key;
	char skey[16];
};

class MyValueType 
{
    public:
        MyValueType()
            : buffer(NULL), size(0), needsDeletion(false)
        {
            DLOG(DINFO, "MyValueType  " << this << " default constructor buffer NULL size 0");
        }

        MyValueType(char* buffer, uint32 size)
            : buffer(buffer),
            size(size),
            needsDeletion(false)
        {
            DLOG(DINFO, "MyValueType  " << this << " constructor buffer " << hex << buffer << dec << " size " << size);
        }

        MyValueType(MyValueType const& other)
            : buffer(other.buffer),
            size(other.size),
            needsDeletion(false)
        {
            DLOG(DINFO, "MyValueType  " << this << " copy constructor buffer " << buffer << " size " << this->size);
        }

        ~MyValueType()
        {
            if (this->size == sizeof(uint64))
            {
                DLOG(DINFO, "destructor this " << this << " key " << *(reinterpret_cast<uint64 *>(this->buffer)));
            }

            if (this->needsDeletion)
            {
                delete [] this->buffer;
            }
        }

        char const* getBuffer() const
        {
            return (this->buffer);
        }

        uint32 getSerializedSize() const
        {
            return (this->size + sizeof(this->size));
        }

        void serialize(char* outBuf) const
        {
            memcpy(outBuf, &this->size, sizeof(this->size));
            memcpy(outBuf + sizeof(this->size), this->buffer, this->size);
        }

        void deserialize(char const* inBuf)
        {
            memcpy(&this->size, inBuf, sizeof(this->size));
            this->buffer = new char[this->size];
            this->needsDeletion = true;
            memcpy(this->buffer, inBuf + sizeof(this->size), this->size);
        }

    private:
        char* buffer;
        uint32 size;
        bool needsDeletion;
};

typedef MyKeyType KeyType;
typedef MyValueType ValueType;

struct myHash
{
    inline size_t operator()(KeyType const& x) const
    {
        std::tr1::hash<uint32> hash32;
        return hash32(x.getKey() & 0xFFFFFFFF) ^ hash32(x.getKey() >> 32);
    }
};

boost::mutex keysMutex;
typedef std::tr1::unordered_set<KeyType, myHash > KeySet;
KeySet validKeys(1000000);

queue<int> writeLoadReq;
queue<KeyType> readLoadReq;
queue<KeyType> delLoadReq;

KeyValueStore<KeyType, ValueType >* kvStore;


void wroteOne(KeyType const& ,
        KeyType* key,
        ValueType* value,
        int64 reqStartTime
        )
{
    DLOG(DINFO, " Wrote one request for key ptr " << key << " " << key->getKey());
    {
        DLOG(DINFO, "wroteOne getting keys lock");
        boost::mutex::scoped_lock lck(keysMutex);
        DLOG(DINFO, "wroteOne inserting key " << key->getKey() << " to validKeys");
        validKeys.insert(*key);
        DLOG(DINFO, "wroteOne insert validKeys size " << validKeys.size());
    }
    delete key;
    delete (value);

    int64 reqEndTime = util::FastTime::getMicroseconds();
    //double reqLatency = reqEndTime > reqStartTime ? (reqEndTime - reqStartTime) : (reqStartTime - reqEndTime);
    double reqLatency = reqEndTime - reqStartTime;
    DLOG(DINFO, "wroteOne request key " << key->getKey());

    {
        bool finished = false;
        uint64 lclPendingWrites;
        uint64 lclPendingReqs;
        {
            DLOG(DINFO, "wroteOne getting write lock");
            boost::mutex::scoped_lock lck(writeMut);
            totalWriteLatency += reqLatency;
            totalWriteLatencySqr += reqLatency * reqLatency;
            ++totalWritten;
            lclPendingWrites = --pendingWrites;
            DLOG(DINFO, "finished one write totalWritten " << totalWritten << " pendingWrites " << lclPendingWrites);
            if (writeFinished)
            {
                finished = true;
            }
        }

        {
            DLOG(DINFO, "wroteOne getting submission lock");
            boost::mutex::scoped_lock wlck(submissionMutex);
            lclPendingReqs = --pendingReqs;
            avgCalculator.addNext(reqLatency);
            updateLatencyBuckets(reqLatency);
            globalLatency+=reqLatency;
            DLOG(DINFO, "wroteOne pendingReqs " << lclPendingReqs);
        }
        DLOG(DINFO, "wroteOne pendingReqs " << pendingReqs);
        if (finished)
        {
            if (lclPendingWrites == 0)
            {
                DLOG(DINFO, "writer thread finished true pendingWrites 0");
                DLOG(DINFO, "wroteOne getting write lock again");
                boost::mutex::scoped_lock lck(writeMut);
                writeCond.notify_one();
                DLOG(DINFO, "wroteOne notifying");
            }
            if (lclPendingReqs == 0)
            {
                DLOG(DINFO, "writer thread pendingReqs 0 signalling waitCond");
                waitCond.signal();
            }
        }
    }
}

void writeOneFailure(KeyType const& key, ExceptionHolder ex, 
        ValueType* value)
{
    delete(const_cast<KeyType *>(&key));
    delete(value);
    try
    {
        ex.throwIt();
    }
    catch (OutOfSpaceException&)
    {
        printStats();
    }
    FAILURE("writeOneFailure " << ex);
}

void preWroteOne(KeyType const& ,
        KeyType* key,
        ValueType* value)
{
    {
        boost::mutex::scoped_lock lck(keysMutex);
        DLOG(DINFO, "prewroteOne inserting key " << key->getKey() << " to validKeys");
        validKeys.insert(*key);
        DLOG(DINFO, "prewroteOne insert validKeys size " << validKeys.size());
    }
    delete key;
    delete value;
    {
        boost::mutex::scoped_lock lck(submissionMutex);
        --numPrewrites;
        if (numPrewrites == 0 && prewritesIssued)
        {
            lck.unlock();
            prewriteCond.signal();
        }
    }
}

void preWriteData()
{
    if (preWriteSize > 0)
    {
        cout << "KVZone prewriting data: " << endl;
        uint64 blockSizeIndex = 0;
        uint64 totalPreWritten = 0;
        for (uint64 count = 0; totalPreWritten < preWriteSize; blockSizeIndex++, count++)
        {
            if (blockSizeIndex == NUM_RAND_VALUES)
            {
                blockSizeIndex = 0;
            }

            MyKeyType* key = new MyKeyType(currKey++);
            MyValueType* val = new MyValueType(buffer, blockSizes[blockSizeIndex]);

            DLOG(DINFO, "calling prewrite insert for key " << key->getKey() << " ptr " << key << " key size " << sizeof(MyKeyType) << " value size " << blockSizes[blockSizeIndex]);

            {
                boost::mutex::scoped_lock lck(submissionMutex);
                ++numPrewrites;
		if(totalPreWritten+blockSizes[blockSizeIndex] >= preWriteSize)
			prewritesIssued = true; 
            }

            SubmitStatus s = kvStore->insert(
                    *key,
                    *val,
                    keyValueStore::DURABLE,
                    bind(&preWroteOne, _1, key, val),
                    bind(writeOneFailure, _1, _2, val));

            DASSERT(s.getStatus() == keyValueStore::SUBMISSIONSTATUS_ACCEPTED, "request rejected");

            totalPreWritten += blockSizes[blockSizeIndex];
        }

        {
            boost::mutex::scoped_lock lck(submissionMutex);
            prewritesIssued = true;
        }
        prewriteCond.wait();

        cout << "KVZone prewrite done size (MB) " << (double) (totalPreWritten * 1.0) / (1024 * 1024) << endl;
    }
}

void writeData(int numReq)
{
    for (int i = 0; i < numReq; ++i)
    {
        uint64 blockSizeIndex = currBlockSizeIndex++;
        if (blockSizeIndex == NUM_RAND_VALUES)
        {
            blockSizeIndex = 0;
            currBlockSizeIndex = 0;
        }

        MyKeyType* key = new MyKeyType(currKey++);
        MyValueType* val = new MyValueType(buffer, blockSizes[blockSizeIndex]);

        int64 reqStartTime = util::FastTime::getMicroseconds();
        {
            DLOG(DINFO, "writer thread getting write lock");
            boost::mutex::scoped_lock lck(writeMut);
            ++pendingWrites;
            DLOG(DINFO, "writeData pendingWrites " << pendingWrites);
        }
        {
            DLOG(DINFO, "writer thread getting submission lock");
            boost::mutex::scoped_lock wlck(submissionMutex);
            ++pendingReqs;
            DLOG(DINFO, "writeData pendingReqs " << pendingReqs);
        }
        DLOG(DINFO, "calling insert for key " << key->getKey() << " ptr " << key << " key size " << sizeof(MyKeyType) << " value size " << blockSizes[blockSizeIndex]);
        SubmitStatus s = kvStore->insert(
                *key,
                *val,
                keyValueStore::DURABLE,
                bind(&wroteOne, _1, key, val, reqStartTime),
                bind(writeOneFailure, _1, _2, val));

        DASSERT(s.getStatus() == keyValueStore::SUBMISSIONSTATUS_ACCEPTED, "request rejected");

    }
}

void generateWrites()
{
    for (;;)
    {
        boost::mutex::scoped_lock lck(writeMut);
        bool empty = false;
        while ( (empty = writeLoadReq.empty()) && !writeFinished)
        {
            DLOG(DINFO, "writer thread entering wait");
            writeCond.wait(lck);
            DLOG(DINFO, "writer thread out of wait");
        }

        if (!empty)
        {
            DLOG(DINFO, "writer thread popping write reqests");
            int reqsDue = writeLoadReq.front();
            writeLoadReq.pop();
            lck.unlock();
            DLOG(DINFO, "writer thread unlocking");
            writeData(reqsDue);
        }
        else
        {
            DLOG(DINFO, "writer thread finished true");
            break;
        }
    }
    {
        boost::mutex::scoped_lock dlck(submissionMutex);
        if (!pendingReqs)
        {
            waitCond.signal();
        }
    }

}

void readOne(KeyType const& , LookupResult result,
        KeyType* key,
        ValueType* value,
        int64 reqStartTime) 
{
    util::unused(result);
    PASSERT( result == FOUND, "value not found for key " << key->getKey());

    int64 reqEndTime = util::FastTime::getMicroseconds();
    //double reqLatency = reqEndTime > reqStartTime ? (reqEndTime - reqStartTime) : (reqStartTime - reqEndTime);
    double reqLatency = reqEndTime - reqStartTime;

    DLOG(DINFO, "readOne request key " << key->getKey());

    delete value;
    {
        DLOG(DINFO, "readOne getting keys lock");
        boost::mutex::scoped_lock lck(keysMutex);
        DLOG(DINFO, "readOne inserting key " << key->getKey() << " to validKeys");
        validKeys.insert(*key);
        DLOG(DINFO, "readOne insert validKeys size " << validKeys.size());
    }
    delete key;

    {
        bool finished = false;
        uint64 lclPendingReads;
        uint64 lclPendingReqs;
        {
            DLOG(DINFO, "readOne getting read lock");
            boost::mutex::scoped_lock lck(readMut);
            totalReadLatency += reqLatency; 
            totalReadLatencySqr += reqLatency * reqLatency; 
            ++totalRead;
            lclPendingReads = --pendingReads;
            DLOG(DINFO, "finished one read totalRead " << totalRead << " pendingReads " << lclPendingReads);
            if (readFinished)
            {
                finished = true;
            }
        }

        {
            DLOG(DINFO, "readOne getting submission lock");
            boost::mutex::scoped_lock rlck(submissionMutex);
            lclPendingReqs = --pendingReqs;
            avgCalculator.addNext(reqLatency);
            updateLatencyBuckets(reqLatency);
            globalLatency+=reqLatency;
            DLOG(DINFO, "readOne pendingReqs " << lclPendingReqs);
        }
        DLOG(DINFO, "readOne pendingReqs " << lclPendingReqs);
        if (finished)
        {
            if (lclPendingReads == 0)
            {
                DLOG(DINFO, "pendingReads 0 signalling readCond");
                DLOG(DINFO, "readOne getting read lock again");
                boost::mutex::scoped_lock lck(readMut);
                readCond.notify_one();
                DLOG(DINFO, "readONe notified");
            }

            if (lclPendingReqs == 0)
            {
                DLOG(DINFO, "pendingReqs 0 signalling waitCond");
                waitCond.signal();
            }
        }
    }
}

void readOneFailure(KeyType const& , ExceptionHolder ex,
        KeyType* key,
        ValueType* value)
{
    util::unused(ex);
    uint64 k = key->getKey();
    util::unused(k);
    delete value;
    delete key;
    FAILURE("value not found for key " << k << ex);
}

void readData(KeyType const& k)
{
    MyKeyType* key = new MyKeyType(k);
    MyValueType* val = new MyValueType();


    int64 reqStartTime = util::FastTime::getMicroseconds();
    {
        DLOG(DINFO, "reader thread getting read lock");
        boost::mutex::scoped_lock lck(readMut);
        ++pendingReads;
        DLOG(DINFO, "readData pendingReads " << pendingReads);
    }
    {
        DLOG(DINFO, "reader thread getting submission lock");
        boost::mutex::scoped_lock wlck(submissionMutex);
        ++pendingReqs;
        DLOG(DINFO, "readData pendingReqs " << pendingReqs);
    }
    DLOG(DINFO, "readData for key " << key->getKey());
    SubmitStatus s = kvStore->lookup(
            *key,
            *val,
            bind(&readOne, _1, _2, key, val, reqStartTime),
            bind(readOneFailure, _1, _2, key, val));

    DASSERT(s.getStatus() == keyValueStore::SUBMISSIONSTATUS_ACCEPTED, "request rejected");
}

void generateReads()
{
    for(;;)
    {
        boost::mutex::scoped_lock lck(readMut);
        bool empty = false;
        while ( (empty = readLoadReq.empty()) && !readFinished)
        {
            DLOG(DINFO, "reader thread entering wait");
            readCond.wait(lck);
            DLOG(DINFO, "reader thread out of wait");
        }
        if (!empty)
        {
            DLOG(DINFO, "reader thread popping read request");
            KeyType rkey (readLoadReq.front());
            readLoadReq.pop();
            lck.unlock();
            DLOG(DINFO, "reader thread unlocking");
            readData(rkey);
        }
        else
        {
            DLOG(DINFO, "reader thread finished");
            break;
        }
    }
    {
        boost::mutex::scoped_lock dlck(submissionMutex);
        if (!pendingReqs)
        {
            waitCond.signal();
        }
    }

}

void deletedOne(KeyType const&, KeyType* k, int64 reqStartTime) 
{
    DLOG(DINFO, "deletedOne request key " << k->getKey());

    delete k;

    int64 reqEndTime = util::FastTime::getMicroseconds();
    //double reqLatency = reqEndTime > reqStartTime ? (reqEndTime - reqStartTime) : (reqStartTime - reqEndTime);
    double reqLatency = reqEndTime - reqStartTime;

    {
        bool finished = false;
        uint64 lclPendingDeletes;   
        uint64 lclPendingReqs;
        {
            DLOG(DINFO, "deletedOne getting del lock");
            boost::mutex::scoped_lock lck(delMut);
            DLOG(DINFO, "deletedOne got del lock");
            totalDeleteLatency += reqLatency; 
            totalDeleteLatencySqr += reqLatency * reqLatency; 
            ++totalDeleted;
            lclPendingDeletes = --pendingDeletes;
            DLOG(DINFO, "finished one deletion totalDeleted " << totalDeleted << " pendingDeletes " << lclPendingDeletes);
            if (delFinished)
            {
                finished = true;
            }
        }
        {
            DLOG(DINFO, "deletedOne getting submission lock");
            boost::mutex::scoped_lock dlck(submissionMutex);
            lclPendingReqs = --pendingReqs;
            avgCalculator.addNext(reqLatency);
            updateLatencyBuckets(reqLatency);
            globalLatency+=reqLatency;
            DLOG(DINFO, "deletedONe pendingReqs " << lclPendingReqs);
        }
        DLOG(DINFO, "deletedOne pendingReqs " << lclPendingReqs); 
        if (finished)
        {
            if (lclPendingDeletes == 0)
            {
                DLOG(DINFO, "deleter thread pendingDeletes 0 notifying delCond");
                DLOG(DINFO, "deletedOne getting del lock again");
                boost::mutex::scoped_lock lck(delMut);
                DLOG(DINFO, "deletedOne got del lock again");
                delCond.notify_one();
                DLOG(DINFO, "deltedOne notifying");
            }

            if (lclPendingReqs == 0)
            {
                DLOG(DINFO, "deleter thread pendingReqs 0 signalling waitCond");
                waitCond.signal();
            }
        }
    }
}

void deleteOneFailure(KeyType const& , ExceptionHolder ex, KeyType* k)
{
    util::unused(ex);
    uint64 key = k->getKey();
    util::unused(key);
    delete k;
    FAILURE("value not found for key " << key << " " << ex);
}

void delData(KeyType const& k)
{
    MyKeyType* key = new MyKeyType(k);

    int64 reqStartTime = util::FastTime::getMicroseconds();
    {
        DLOG(DINFO, "deleter thread delData getting del lock");
        boost::mutex::scoped_lock lck(delMut);
        DLOG(DINFO, "deleter thread delData got del lock");
        ++pendingDeletes;
        DLOG(DINFO, "delData got lock pendingDeletes " << pendingDeletes);
    }
    DLOG(DINFO, "delData for key " << k.getKey() << " pendingDeletes " << pendingDeletes);
    {
        DLOG(DINFO, "deleter thread locking submission lock");
        boost::mutex::scoped_lock wlck(submissionMutex);
        ++pendingReqs;
        DLOG(DINFO, "delData pendingReqs " << pendingReqs );
    }
    SubmitStatus s = kvStore->remove(
            *key,
            bind(&deletedOne, _1, key, reqStartTime),
            bind(deleteOneFailure, _1, _2, key));

    DASSERT(s.getStatus() == keyValueStore::SUBMISSIONSTATUS_ACCEPTED, "request rejected");
}

void generateDeletes()
{
    for (;;)
    {
        DLOG(DINFO, "deleter thread generateDeletes getting del lock");
        boost::mutex::scoped_lock lck(delMut);
        DLOG(DINFO, "deleter thread generateDeletes got del lock");
        bool empty = false;
        while ( (empty = delLoadReq.empty()) && !delFinished)
        {
            DLOG(DINFO, "deleter thread entering wait delLoadReq size " << delLoadReq.size() << " delFinished " << delFinished);
            delCond.wait(lck);
            DLOG(DINFO, "deleter thread out of wait");
        }

        if (!empty)
        {
            DLOG(DINFO, "deleter thread popping delete request");
            KeyType dkey (delLoadReq.front());
            delLoadReq.pop();
            lck.unlock();
            DLOG(DINFO, "deleter thread unlocking");
            delData(dkey);

        }
        else
        {
            DLOG(DINFO, "deleter thread finished");
            break;
        }
    }
    {
        boost::mutex::scoped_lock dlck(submissionMutex);
        if (!pendingReqs)
        {
            waitCond.signal();
        }
    }
}

void generateLoad()
{
    uint64 totalReadSubmitted = 0;
    uint64 totalWriteSubmitted = 0;
    uint64 totalDelSubmitted = 0;
    uint64 totalSubmitted = 0;

    uint64 startTime = util::FastTime::getMillisecondsSinceEpoch();

    uint64 readReqsDue = 0;
    uint64 writeReqsDue = 0;
    uint64 delReqsDue = 0;
    uint64 reqsDueThisCycle = 0;

    uint64 lastTime = 0;
    uint64 initiateTargetIopsChage = 0;
    uint64 latTargetHit = 0;

    uint percentDone = 0;
    uint lastPercentDone = 0;
    ProgressBar progressBar;
    bool updateProgressBar = true;

    struct timespec sleepTime = { 0, 10000000 };
    for (; totalSubmitted < totalRequests;)
    {
        nanosleep(&sleepTime, NULL);

        lastPercentDone = percentDone;
        percentDone = (totalSubmitted * 1000)/totalRequests;
        char progressBarText[50]; 
        if (percentDone != lastPercentDone)
        {
            progressBar.updatePercent(percentDone/10.);
            updateProgressBar = true;
        }

        uint64 milliSecondsElapsed = util::FastTime::getMillisecondsSinceEpoch() - startTime; 
        if (latencyMode)
        {
            double lat = avgCalculator.getAverage();
            double alat = globalLatency/(totalWritten + totalRead + totalDeleted);

            snprintf(progressBarText, 50, "clat:%02.0f alat:%02.0f IOPs:%llu", lat, alat, generatorIops);
            progressBar.updateText(string(progressBarText));

            if (initiateTargetIopsChage >  min(changeIopsAfter,maxChangeIopsAfter))
            {
                initiateTargetIopsChage = 0;
                boost::mutex::scoped_lock lck(submissionMutex);
                DLOG(DINFO, "got avg latency " << lat);
                DLOG(DINFO, "check if target iops adjustment is needed");
                if (iopsGenerator->needsAdjustment(lat))
                {
                    changeIopsAfter = generatorIops/2;
                    generatorIops = iopsGenerator->adjustTargetIops(lat, alat, latTargetHit);
                    if (generatorIops < 10) THROW(IoException, "cannot reach latency target.");
                    latTargetHit = 0;
                    DLOG(DINFO, "adjustment is needed new generatorIops " << generatorIops);
                }
                else
                {
                    latTargetHit++;
                    DLOG(DINFO, "no iops adjustment needed");
                }
            }

            reqsDueThisCycle = min((uint64)(ceil((milliSecondsElapsed - lastTime) * (generatorIops/ 1000.))), (totalRequests - totalSubmitted));
            DLOG(DINFO, "generator thread time elapsed " << (milliSecondsElapsed - lastTime) << " reqsDueThisCycle " << reqsDueThisCycle << " totalDelSubmitted " << totalDelSubmitted);

            readReqsDue = (uint64)(ceil((readPct * reqsDueThisCycle * 1.0) / (100 * 1.0))); 
            if (readPct > 0) readReqsDue = max(readReqsDue, (uint64)1);
            writeReqsDue = (uint64)(ceil((((100 - readPct) * reqsDueThisCycle) * 1.0 / 100) / 2));
            if (readPct < 100) writeReqsDue = max(writeReqsDue, (uint64)1);
            delReqsDue = writeReqsDue;
            DLOG(DINFO, "generator thread time elapsed " << (milliSecondsElapsed - lastTime) << " reqsDueThisCycle " << reqsDueThisCycle << " writeReqsDue " << writeReqsDue << " readReqsDue " << readReqsDue << " delReqsDue " << delReqsDue);
            lastTime = milliSecondsElapsed;
        }
        else
        {
            reqsDueThisCycle = min((uint64)(ceil((milliSecondsElapsed - lastTime) * (generatorIops/ 1000.))), totalRequests);
            DLOG(DINFO, "generator thread milliSecondsElapsed " << milliSecondsElapsed << " reqsDueThisCycle " << reqsDueThisCycle << " totalDelSubmitted " << totalDelSubmitted);

            readReqsDue = (uint64)(ceil((readPct * reqsDueThisCycle * 1.0) / (100 * 1.0))); 
            writeReqsDue = (uint64)(ceil((((100 - readPct) * reqsDueThisCycle) * 1.0 / 100) / 2));
            delReqsDue = writeReqsDue;

            readReqsDue = (readReqsDue > totalReadSubmitted)?(readReqsDue - totalReadSubmitted):0;
            writeReqsDue = (writeReqsDue > totalWriteSubmitted)?(writeReqsDue - totalWriteSubmitted):0;
            delReqsDue = (delReqsDue > totalDelSubmitted)?(delReqsDue - totalDelSubmitted):0;
            DLOG(DINFO, "milliSecondsElapsed " << milliSecondsElapsed << " reqsDueThisCycle " << reqsDueThisCycle << " writeReqsDue " << writeReqsDue << " readReqsDue " << readReqsDue << " delReqsDue " << delReqsDue);

        }

        DLOG(DINFO, "milliSecondsElapsed " << milliSecondsElapsed << " reqsDueThisCycle " << reqsDueThisCycle << " writeReqsDue " << writeReqsDue << " readReqsDue " << readReqsDue << " delReqsDue " << delReqsDue);

        KeySet tempKeys;

// TODO: Fix Read/Delete
// For now, we reorder so reads happen at a higher priority than deletes.
        DLOG(DINFO, "generateLoad getting keys lock");
        boost::mutex::scoped_lock klck(keysMutex);
        DLOG(DINFO, "generateLoad got keys lock");


        {
            boost::mutex::scoped_lock lck(readMut);
            for (uint32 i = 0; i < readReqsDue; ++i)
            {
                if (validKeys.empty())
                {
                    break;
                }
                PASSERT(validKeys.size(), "validKeys empty");
                KeySet::iterator i = validKeys.begin();
                KeyType key(*i);
                DLOG(DINFO, "generateLoad read erasing key " << key.getKey() << " from validKeys");
                validKeys.erase(i);
                DLOG(DINFO, "generate read erase validKeys size " << validKeys.size()); 
                readLoadReq.push(key);
                ++totalReadSubmitted;
                DLOG(DINFO, "submitted read totalReadSubmitted " << totalReadSubmitted);
            }
            DLOG(DINFO, "generateLoad submitted " << readReqsDue << " read requests signalling readCond");
        }

        {
            DLOG(DINFO, "generateLoad getting del lock");
            boost::mutex::scoped_lock lck(delMut);
            DLOG(DINFO, "generateLoad got del lock delReqsDue " << delReqsDue);
            for (uint32 i = 0; i < delReqsDue; ++i)
            {
                if (validKeys.empty())
                {
                    break;
                }
                PASSERT(validKeys.size(), "validKeys empty");
                KeySet::iterator i = validKeys.begin();
                KeyType key(*i);
                DLOG(DINFO, "generateLoad del erasing key " << key.getKey() << " from validKeys");
                validKeys.erase(i);
                DLOG(DINFO, "generateLoad del erase validKeys size " << validKeys.size()); 
                delLoadReq.push(key);
                ++totalDelSubmitted;
                DLOG(DINFO, "submitted deletion totalDelSubmitted " << totalDelSubmitted);
            }
            DLOG(DINFO, "generateLoad submitted " << delReqsDue << " delete requests signalling delCond");
        }

        klck.unlock();

        if (writeReqsDue != 0)
        {
            boost::mutex::scoped_lock lck(writeMut);
            writeLoadReq.push(writeReqsDue);
            DLOG(DINFO, "generateLoad submitted " << writeReqsDue << " write requests signalling writeCond");
            lck.unlock();
            totalWriteSubmitted += writeReqsDue;
            DLOG(DINFO, "submitted write totalWriteSubmitted " << totalWriteSubmitted);
        }

        initiateTargetIopsChage += (totalReadSubmitted + totalWriteSubmitted + totalDelSubmitted) - totalSubmitted;
        totalSubmitted  = totalReadSubmitted + totalWriteSubmitted + totalDelSubmitted; 

        {
            DLOG(DINFO, "generateLoad getting del lock");
            boost::mutex::scoped_lock lck(delMut);
            DLOG(DINFO, "generateLoad got del lock");
            delCond.notify_one();
        }
        {
            boost::mutex::scoped_lock lck(readMut);
            readCond.notify_one();
        }
        {
            boost::mutex::scoped_lock lck(writeMut);
            writeCond.notify_one();
        }

        if (updateProgressBar)
        {
            updateProgressBar = false;
            progressBar.refresh();
        }
    }
    DLOG(DINFO, "generateLoad out of main loop totalSubmitted " << totalSubmitted << " totalRequests " << totalRequests);

    {
        boost::mutex::scoped_lock lck(submissionMutex);
        DLOG(DINFO, "finished set to true signalling writeCond");
        {
            boost::mutex::scoped_lock lck(writeMut);
            writeFinished = true;
            writeCond.notify_one();
        }
        {
            boost::mutex::scoped_lock lck(readMut);
            readFinished = true;
            readCond.notify_one();
        }
        {
            DLOG(DINFO, "generateLoad getting del lock");
            boost::mutex::scoped_lock lck(delMut);
            DLOG(DINFO, "generateLoad got del lock");
            delFinished = true;
            delCond.notify_one();
        }
    }
}

int main(int argc, char*argv[])
{
    string storeSizeStr = "6GB";
    string minBlockSizeStr = "32KB";
    string maxBlockSizeStr = "256KB";
    string kvTypeStr = "";
    string devicePath = "";

    if (argc < 2)
    {
        usage(argv[0]);
        exit(1);
    }

    int c = 0;
    while ((c = getopt(argc, argv, "p:z:s:P:L:v:m:x:o:t:e:r:c:i:w:u:h:a:k")) != -1)
    {
        switch (c)
        {
//            case 'g':
//                logFilePath = boost::lexical_cast<string>(optarg);
//                break;
            case 'p':
                devicePath = boost::lexical_cast<string>(optarg);
                break;
            case 'z':
                storeSizeStr = boost::lexical_cast<string>(optarg);
                storeSize = util::byteStringToNumber(storeSizeStr, KB);
                break;
            case 'P':
                preWriteSize = strtoll(optarg, NULL, 0);
                if (preWriteSize > 100)
                {
                    usage(argv[0]);
                    exit(1); 
                }
                preWriteSize = (preWriteSize * storeSize / 100);
                break;
            case 's':
                totalRequests = strtoll(optarg, NULL, 0); 
                break;
            case 'v':
                deviation = strtol(optarg, NULL, 0);
                break;
            case 'm':
                minBlockSizeStr = boost::lexical_cast<string>(optarg);
                minBlockSize = util::byteStringToNumber(minBlockSizeStr, KB);
                break;
            case 'x':
                maxBlockSizeStr = boost::lexical_cast<string>(optarg);
                maxBlockSize = util::byteStringToNumber(maxBlockSizeStr, KB);
                break;
            case 'o':
                generatorIops = strtoll(optarg, NULL, 0);
                iopsMode = true;
                break;
            case 'a':
                targetLatency = strtod(optarg, NULL);
                latencyMode = true;
                generatorIops = startIops;
                iopsGenerator = new TargetIOPSGenerator(startIops, targetLatency);
                break;
            case 'L':
                readPct = strtol(optarg, NULL, 0);
                break;
            case 't':
                kvTypeStr = boost::lexical_cast<string>(optarg);
                if (kvTypeStr == "bdb")
                {
                    kvType = BERKELEYDB;
                }
                else if (kvTypeStr == "bdb2")
                {
                    kvType = BERKELEYDB2;
                }
                else if (kvTypeStr == "tc")
                {
                    kvType = TOKYOCABINET;
                }
                else if (kvTypeStr == "sqlite")
                {
                    kvType = SQLITE;
                }
                else if (kvTypeStr == "mem")
                {
                    kvType = MEMMAP;
                }
                else if (kvTypeStr == "noop")
                {
                    kvType = NOOP;
                }
                else
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            case 'r':
                numThreads = static_cast<uint16_t>(strtol(optarg, NULL, 0));
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
            default:
                usage(argv[0]);
                exit(1);
        }
    };

    if (latencyMode == false && iopsMode == false)
    {
        cerr << " either target IOPS or target latency has to be specified" << endl;
        usage(argv[0]);
        exit(1);
    }

    if (latencyMode == true && iopsMode == true)
    {
        cerr << " only one of target IOPS and target latency can be specified" << endl;
        usage(argv[0]);
        exit(1);
    }

    util::FastTime::startFastTimer(true); // high res timer needed

    LOG(INFO, "Starting test");
    LocalKeyValueStoreConfigs* config = new LocalKeyValueStoreConfigs(devicePath, 
            numThreads,
            storeSize,
            kvType);

    kvStore = new KeyValueStore<KeyType, ValueType>(*config);

    buffer = new char[maxBlockSize];
    bzero(buffer, maxBlockSize);

    generateBlockSizes();
    preWriteData();
    kvfs_sync();
    boost::thread_group threads;
    {
        boost::shared_ptr<boost::thread> genThread,writeThread,readThread,deleteThread;
        threads.create_thread(boost::bind(&generateWrites));
        threads.create_thread(boost::bind(&generateReads));
        threads.create_thread(boost::bind(&generateDeletes));

        startTime = util::FastTime::getMillisecondsSinceEpoch();
        threads.create_thread(boost::bind(&generateLoad));


        DLOG(DINFO, "waiting on waitCond");
        waitCond.wait();
        endTime = util::FastTime::getMillisecondsSinceEpoch();
        printStats();

        threads.join_all();
    }

    if (latencyMode)
    {
        delete iopsGenerator;
    }
    delete [] buffer;
    delete kvStore;
    delete config;


    LOG(INFO, "finished test");

    return errorCode;
}
