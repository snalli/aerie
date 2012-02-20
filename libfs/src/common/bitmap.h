/**
 * \brief Fixed size bitset container
 */

#ifndef __STAMNOS_BITMAP_H
#define __STAMNOS_BITMAP_H

#include <stdint.h>

typedef uint64_t bitmap64;
typedef uint32_t bitmap32;
typedef uint16_t bitmap16;
typedef uint8_t  bitmap8;

template<class T>
class Bitmap {
public:
	static inline T Set(int bit) { return 1 << bit; }
	static inline bool IsSet(T bm, int bit) {
		return (((1 << bit) & bm) ? true: false);
	}
	static inline T Reset(T bm, int bit) {
    	return ((~(1 << bit)) & bm);
    }
};


#define BITMAP_SET(bit) (1 << (bit))
#define BITMAP_ISSET(bm, bit) (((1 << (bit)) & bm) ? true: false)

//Bitmap<b64>::Set(1);

#endif /* __STAMNOS_BITMAP_H */
