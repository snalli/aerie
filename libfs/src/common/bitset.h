/**
 * \brief Dynamic size bitset container.
 *
 * Boost provides a dynamic_bitset implementation. Nevertheless, we implement our 
 * own because we want to use it in our persistent data structures.  
 * 
 * To use the Boost implementation we would have to:
 * 1) implement an STL allocator that allocates persistent space
 * 2) modify it to perform consistent updates.
 */

#ifndef __STAMNOS_BITSET_H
#define __STAMNOS_BITSET_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

class DynamicBitSet {
public:
	class BitReference {
	public:
		BitReference(char* byte, uint64_t pos) 
			: byte_(byte),
			  pos_(pos)
		{ }

		BitReference& operator=(bool value) {
			uint64_t idx = pos_ / sizeof(char);
			int      offset = pos_ & (sizeof(char) - 1); // pos % sizeof(char);
			set_bit(idx, offset, value);
		}

		operator bool() {
			uint64_t idx = pos_ / sizeof(char);
			int      offset = pos_ & (sizeof(char) - 1); // pos % sizeof(char);
			return byte_[idx] & (1 << offset);
		}

	private:
		void set_bit(int index, int offset, bool val) {
			if (val) {
				byte_[index] |= (1 << offset);
			} else {
				byte_[index] &= ~(1 << offset);
			}
		}

		char*    byte_;
		uint64_t pos_;
	};

	static DynamicBitSet* Make(void* b, int size) {
		DynamicBitSet* bset = reinterpret_cast<DynamicBitSet*>(b);
		bset->size_ = size;
		bset->Reset();
		return bset;
	}
	
	static DynamicBitSet* Load(void* b) {
		return reinterpret_cast<DynamicBitSet*>(b);
	}
	
	void Reset() {
		memset(byte(), 0, size_);
	}

	void Reset(uint64_t pos) {
		BitReference ref = BitReference(byte(), pos);
		ref = false;
	}
	
	void Set(uint64_t pos, bool value = true) {
		BitReference ref = BitReference(byte(), pos);
		ref = value;
	}

	bool IsSet(uint64_t pos) {
		BitReference ref = BitReference(byte(), pos);
		return ref == true;
	}

	uint64_t Size() { return size_; }

	BitReference operator[](int pos)
	{
		return BitReference(byte(), pos);
	}

	// returns the number of bytes the bitset will occupy to sore nbits
	static uint64_t Sizeof(int nbits) {
		return sizeof(DynamicBitSet) + ( (nbits / (8*sizeof(char))) + (nbits % sizeof(char) ? 1 : 0));
	}

private:
	char* byte() {
		return (char*) this + sizeof(DynamicBitSet);
	}

	uint64_t size_;
};


#endif // __STAMNOS_BITSET_H
