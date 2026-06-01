/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _UTIL_STATISTICS_H
#define _UTIL_STATISTICS_H
/*
 * Object-Oriented Server Framework Library
 */

#include <boost/ptr_container/ptr_map.hpp>
#include <util/types.h>

namespace util
{
	/**
     * Stats share the same semantics:
     * 
     * totals do not reflect recent reports
     * 
     * updateTotal must be called prior to resetRecent, if totals
     * are to be updated
     */

	/**
     * This statistic, monitors a certain value.
     * It reports maximal, minimal and average value.
     * It also reports the number of samples.
     */
	template <typename TValue>
	class ValueStat
	{
	public:
		ValueStat();

		void report(const TValue& value);
		
		/**
         * reports adding to the last reported value
         */
		void reportAdd(const TValue& value);

        /**
         * reports substracting to the last reported value
         */
		void reportSub(const TValue& value);

		TValue const& getCurrent() const;

		TValue const& getTotalMax() const;
		TValue const& getTotalMin() const;
		uint64 const& getTotalNumOfSamples() const;

		TValue const& getRecentMax() const;
		TValue const& getRecentMin() const;
		TValue const& getRecentAvg() const;
		inline TValue const& getRecentSum() const;
		uint64 const& getRecentNumOfSamples() const;

		void updateTotal();
		void resetRecent();
		void getRecentDataFrom(const ValueStat& stat);
		ValueStat& operator+=(ValueStat const & stat);
	private:
		TValue current;
		TValue totalMax;
		TValue totalMin;
		uint64 totalNumOfSamples;

		TValue recentMax;
		TValue recentMin;
		TValue recentSum;
		mutable TValue recentAvg;
		uint64 recentNumOfSamples;
	};

	/**
     * Sum Statistic, monitors the sum of a value
	 */
	template <typename TValue>
	class SumStat
	{
	public:
		SumStat();

		void add(const TValue& value);

		const TValue& getTotalSum() const;
		const TValue& getRecentSum() const;

		void updateTotal();
		void resetRecent();
		void getRecentDataFrom(const SumStat& stat);
		SumStat& operator+=(SumStat const & stat);
	private:
		TValue total;
		TValue recent;
	};

// ======================================================================================
// implementation
// ======================================================================================

		//======================================================================
	// ValueStat
	//======================================================================

	template <typename TValue>
	inline ValueStat<TValue>::ValueStat() :
			current(),
			totalMax(),
			totalMin(),
			totalNumOfSamples(),
			recentMax(),
			recentMin(),
			recentSum(),
			recentAvg(),
			recentNumOfSamples()
	{
	}

	template <typename TValue>
	void ValueStat<TValue>::report(const TValue& value)
	{
		current = value;

		if (recentNumOfSamples == 0)
		{
			recentMax = current;
			recentMin = current;
		}
		else
		{
			if (current > recentMax)
			{
				recentMax = current;			
			}
			else if (current < recentMin)
			{
				recentMin = current;
			}
		}

		recentNumOfSamples++;

		recentSum += current;
	}

	template <typename TValue>
	void ValueStat<TValue>::reportAdd(const TValue& value)
	{
		report(current + value);
	}

	template <typename TValue>
	void ValueStat<TValue>::reportSub(const TValue& value)
	{
		report(current - value);
	}

	template <typename TValue>
	inline TValue const& ValueStat<TValue>::getCurrent() const
	{
		return this->current;
	}

	template <typename TValue>
	inline TValue const& ValueStat<TValue>::getTotalMax() const
	{
		return this->totalMax;
	}

	template <typename TValue>
	inline TValue const& ValueStat<TValue>::getTotalMin() const
	{
		return this->totalMin;
	}

	template <typename TValue>
	inline uint64 const& ValueStat<TValue>::getTotalNumOfSamples() const
	{
		return this->totalNumOfSamples;
	}

	template <typename TValue>
	inline TValue const& ValueStat<TValue>::getRecentMax() const
	{
		return this->recentMax;
	}

	template <typename TValue>
	inline TValue const& ValueStat<TValue>::getRecentMin() const
	{
		return this->recentMin;
	}

    template <typename TValue>
	inline TValue const& ValueStat<TValue>::getRecentSum() const
	{
		return this->recentSum;
    }

	template <typename TValue>
	inline TValue const& ValueStat<TValue>::getRecentAvg() const
	{
		if (this->recentNumOfSamples == 0)
		{
			recentAvg = TValue();
		}
		else
		{
			recentAvg = this->recentSum / this->recentNumOfSamples;
		}
		
		return recentAvg;
	}

	template <typename TValue>
	inline uint64 const& ValueStat<TValue>::getRecentNumOfSamples() const
	{
		return this->recentNumOfSamples;
	}

	template <typename TValue>
	inline void ValueStat<TValue>::updateTotal()
	{
		if (this->totalNumOfSamples == 0)
		{
			this->totalMax = this->recentMax;
			this->totalMin = this->recentMin;
		}
		else
		{
			if (this->recentMax > this->totalMax)
			{
				this->totalMax = this->recentMax;
			}
			if (this->recentMin < this->totalMin)
			{
				this->totalMin = this->recentMin;
			}
		}

		totalNumOfSamples += recentNumOfSamples;
	}

	template <typename TValue>
	inline void ValueStat<TValue>::resetRecent()
	{
		this->recentMax = TValue();
		this->recentMin = TValue();
		this->recentSum = TValue();
		this->recentNumOfSamples = 0;
	}

	template <typename TValue>
	inline void ValueStat<TValue>::getRecentDataFrom(const ValueStat& stat)
	{
		if (this->recentNumOfSamples == 0)
		{
			this->recentMax = stat.recentMax;
			this->recentMin = stat.recentMin;
		}
		else
		{
			if (stat.recentMax > this->recentMax)
			{
				this->recentMax = stat.recentMax;			
			}
			else if (stat.recentMin < this->recentMin)
			{
				this->recentMin = stat.recentMin;
			}
		}

		this->recentSum += stat.recentSum;
		this->recentNumOfSamples += stat.recentNumOfSamples;
	}

	template <typename TValue>
	inline ValueStat<TValue>& ValueStat<TValue>::operator+=(ValueStat const & stat)
	{
		this->current += stat.current;
		if (this->totalMax < stat.totalMax)
		{
			this->totalMax = stat.totalMax;
		}
		if (this->totalMin > stat.totalMin)
		{
			this->totalMin = stat.totalMin;
		}
		this->totalNumOfSamples += stat.totalNumOfSamples;
		if (this->recentMax < stat.recentMax)
		{
			this->recentMax = stat.recentMax;
		}
		if (this->recentMin > stat.recentMin)
		{
			this->recentMin = stat.recentMin;
		}
		this->recentSum += stat.recentSum;
		this->recentNumOfSamples += stat.recentNumOfSamples;
		this->getRecentAvg();
		return *this;
	}

	//======================================================================
	// SumStat
	//======================================================================

	template <typename TValue>
	inline SumStat<TValue>::SumStat() : total(), recent()
	{
		
	}

	template <typename TValue>
	inline void SumStat<TValue>::add(const TValue& value)
	{
		this->recent += value;
	}

	template <typename TValue>
	inline void SumStat<TValue>::updateTotal()
	{
		this->total += this->recent;
	}

	template <typename TValue>
	inline void SumStat<TValue>::resetRecent()
	{
		this->recent = TValue();
	}

	template <typename TValue>
	inline const TValue& SumStat<TValue>::getTotalSum() const
	{
		return total;
	}

	template <typename TValue>
	inline const TValue& SumStat<TValue>::getRecentSum() const
	{
		return recent;
	}

	template <typename TValue>
	inline void SumStat<TValue>::getRecentDataFrom(const SumStat& stat)
	{
		this->recent += stat.recent;
	}

	template <typename TValue>
	inline SumStat<TValue>& SumStat<TValue>::operator+=(SumStat const &stat)
	{
		this->total += stat.total;
		this->recent += stat.recent;
		return *this;
	}


}//namespace util

#endif //_UTIL_STATISTICS_H
