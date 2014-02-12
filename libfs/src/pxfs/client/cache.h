/*
 * This file implements the namespace cache for pxfs.
 * It also has $defs that control debug options, upper limit on cache etc.
 * Author : Sanketh Nalli
 * Year : 2013
 * Place : University of Wisconsin-Madison
 *
 */
#ifndef __CACHE__H
#define __CACHE__H


        #include <sys/syscall.h>

	#define CONFIG_CACHE   		// enables caching in libfs/src/pxfs/client/namespace.cc
        #define N_CACHE 4
	//#define CONFIG_CALLBACK		// enables cache flush on call back from TFS
	//#define DEBUG_SEGFAULT 	// prints out all daddr in src/scm/scm/scm_memcpy.cc to daddr.txt
	//#define FTRACE		// enable ftrace in filebench/flowop.c
	//#define CONFIG_DEBUG  	// prints out a log of all inserts/deletes from cache and opens/writes/reads/close to debug.txt
	//#define FAST_SPIDER_LOCK_PATH
	//#define MAX_INST 400000 	// sets the upper limit on the cache. Set it high enough so that we never reach it
	//#define LATENCY_PRESENT	// Enables/disable latency on writes to SCM device	
        #define RW_LOCK         // Toggle between rw_lock and spin_lock that guards the cache
	#define SMART_LKUP
	#define LCK_MGR
	#define stats unsigned long
        #define s_tid      syscall(SYS_gettid)
#endif

#ifdef CONFIG_DEBUG
        #include <syslog.h>

   #define s_log(...) printf("\n"); \
                                printf(__VA_ARGS__);
        //#define s_log(...) syslog(LOG_DEBUG, __VA_ARGS__);

#else
        #define s_log(...) {}
#endif

namespace osd {
namespace common {
class Cache {

                int             DENTRY_SIZE;
                pthread_rwlock_t   rwlock;
                pthread_spinlock_t spinlock;
	         
		int pshared;
	struct Dentry {
                Dentry ()
                        : self(NULL),
                          parent(NULL)
                { }

                Dentry (void *self_, void *parent_)
                        : self(self_),
                          parent(parent_)
                { }
                void *self;
                void *parent;
        };


        typedef google::dense_hash_map<std::string, Dentry> DentryCache;
        unsigned long           succEntry, failEntry;
        unsigned long           cacheHit, cacheMiss;
        unsigned long           cacheLookup;

	DentryCache  dentries_;
	bool done;
	int id;
	public:
	 Cache(int self_id)
        {
                done = false;
                id = self_id;
                init_cache();

        }

	Cache() { init_cache(); }

	void init_cache() {
	        dentries_.set_empty_key("__EMPTY_KEY__");
                dentries_.set_deleted_key("__DELETED_KEY__");
                succEntry = 0;
                failEntry=0;
                cacheHit=0;
                cacheMiss=0;
                cacheLookup=0;
                DENTRY_SIZE = 1000;
                pthread_rwlock_init(&rwlock, NULL);
                pthread_spin_init(&spinlock, pshared);
		printf("\n***************************************************\n");
	        printf("\nCache # : %d", id);
                printf("\nPID : %d",getpid());
        //        printf("\nTID : %d \nInitializing NameSpace...\
                        \nCache Hits : %lu \
                        \nCache Miss : %lu \
                        \nCache Accs : %lu \n",s_tid, cacheHit, cacheMiss, cacheLookup);
        //        printf("Cache Inst : %lu\n", succEntry);
                #ifdef RW_LOCK
                printf("\n* Using reader-writer locks to guard cache");
                #else
                printf("\n* Using spin-locks to guard cache");
                #endif
                printf("\n***************************************************\n");

	}

	int lookup_cache(const char* name, void** ip, void**dip) {

		#ifdef RW_LOCK
                       pthread_rwlock_rdlock(&rwlock);
                #else
                       pthread_spin_lock(&spinlock);
                #endif

	        //++cacheLookup;

	        DentryCache::iterator it;
        	if ((dentries_.empty() == false) && ((it = dentries_.find(name)) != dentries_.end())) { 
		// Check if the dentry cache is empty ! Check if it was a hit !
        	        *ip = it->second.self;  
                	*dip = it->second.parent;
			// printf("\nLookup <name : %s, <ip : %p, dip : %p>>", name, *ip, *dip);   
        	        //++cacheHit;

                	s_log("[%ld] %s(S) %s <ip : %p, dip : %p>",s_tid, __func__, name, *ip, *dip);
        	                                              
			 #ifdef RW_LOCK
                       		pthread_rwlock_unlock(&rwlock);
                         #else
                         	pthread_spin_unlock(&spinlock);
                         #endif

                	return 1; // Success - It's a cache hit !
	        }
        	//++cacheMiss;

	        s_log("[%ld] %s(F) %s",s_tid, __func__, name);
		    #ifdef RW_LOCK
                       pthread_rwlock_unlock(&rwlock);
                    #else
                       pthread_spin_unlock(&spinlock);
                    #endif

	        return 0; // Failure - It's a cache miss !

	}

	int insert_cache(const char* name, void* ip, void *dip) {
	
	        #ifdef MAX_INST
                if (succEntry > MAX_INST) {
                        return 0;
                }
	        #endif
                DentryCache::iterator it;
                Dentry dentry;
		#ifdef RW_LOCK
                pthread_rwlock_wrlock(&rwlock);
                #else
                pthread_spin_lock(&spinlock);
                #endif

                dentry = Dentry(ip, dip);
		if(1){
        //        if(((it = dentries_.find(name)) == dentries_.end())) {
                        dentries_.insert(std::pair<std::string, Dentry>(name, dentry));
                   //     ++succEntry;
        	s_log("[%ld] %s(S) %s <ip : %p, dip : %p>",s_tid, __func__, name, ip, dip);


			#ifdef RW_LOCK
                        pthread_rwlock_unlock(&rwlock);
                        #else
                        pthread_spin_unlock(&spinlock);
                        #endif

                        return 1;
                }
	        s_log("[%ld] %s(F) %s",s_tid, __func__, name);

			#ifdef RW_LOCK
                        pthread_rwlock_unlock(&rwlock);
                        #else
                        pthread_spin_unlock(&spinlock);
                        #endif

                return 0;

	}

	int erase_cache(const char* name) {


        	s_log("[%ld] %s %s",s_tid, __func__, name);
	    #ifdef RW_LOCK
            pthread_rwlock_wrlock(&rwlock);
            #else
            pthread_spin_lock(&spinlock);
            #endif

		        dentries_.erase(name);
		#ifdef RW_LOCK
                pthread_rwlock_unlock(&rwlock);
                #else
                pthread_spin_unlock(&spinlock);
                #endif

        	return 0;

	}

	int flush_cache(const char* name) {
/*
           pthread_spin_lock(&spinlock);
                DentryCache::iterator it;
                if ((dentries_.empty() == false)) {
			if (((it = dentries_.find(name)) != dentries_.end())) {

                        	dentries_.erase(name);
                 	       //s_log("[%ld] %s(S) %s <ip : %p, dip : %p>",s_tid, __func__, name, *ip, *dip);

                      	  pthread_spin_unlock(&spinlock);
                        	return 1; // Success - It's a cache hit !
                	}
		}
                pthread_spin_unlock(&spinlock);
*/	
                return 0;

        }

	void cache_stats() {
	

	        pthread_spin_lock(&spinlock);

                if(!done)
                {
		pthread_spin_unlock(&spinlock);
		float cache_hit = cacheHit;
                float cache_lookup = cacheLookup;
                float hit_rate = 100 * (cache_hit / cache_lookup);
                printf("\n***************************************************\n");
                printf("\nPID : %d",getpid());
                printf("\nTerminating NameSpace...");
                printf("\nCache Hits : %lu Hit rate : %0.2f %%", cacheHit, hit_rate);
                printf("\nCache Miss : %lu", cacheMiss);
                printf("\nCache Accs : %lu\n", cacheLookup);
                printf("Cache Inst : %lu\n", succEntry);
                printf("\n***************************************************\n\n\n\n\n");
	  }
                pthread_spin_unlock(&spinlock);


	}

};
} // namespace common
} // namespace osd
