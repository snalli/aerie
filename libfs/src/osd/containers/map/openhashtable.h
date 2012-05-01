#ifndef __STAMNOS_OSD_COMMON_OPENHASHTABLE_H 
#define __STAMNOS_OSD_COMMON_OPENHASHTABLE_H 


#include <assert.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include "bcs/main/common/cdebug.h"
#include "common/errno.h"

typedef unsigned int uint;

#define INITIAL_NUM_SLOTS 32

template <class Session, class Key, class Value, class Traits> class OpenHashTable
{
public:
    struct Slot
    {
		uint hash;
        Key key;
        Value value;
        Slot() : hash(), key(), value() {}
    };

    uint m_numSlots;
    uint m_numKeys;
	uint m_limitKeys;
    Slot *m_slots;
	float m_threshold;
	Slot m_slots_static[INITIAL_NUM_SLOTS]; 

    OpenHashTable(uint numSlots, float threshold)
    {
        assert((numSlots & (numSlots - 1)) == 0);	// Must be a power of two.
        m_numSlots = numSlots;
        m_numKeys = 0;
		m_limitKeys = (uint) (m_numSlots * threshold);
        m_slots = &m_slots_static;
		m_slots = new Slot[m_numSlots];
		m_threshold = threshold;
    }

	// always start with a small directory and then expand
    OpenHashTable(float threshold)
    {
        m_numSlots = INITIAL_NUM_SLOTS;
        m_numKeys = 0;
		m_limitKeys = (uint) (m_numSlots * threshold);
        m_slots = m_slots_static;
		m_threshold = threshold;
    }

    ~OpenHashTable()
    {
		//delete[] m_slots;
    }

	int Lookup (Session* session, const Key &key, Value* val)
    {
		uint hash = Traits::hash(key);
		uint mask = m_numSlots - 1;
		uint index = hash & mask;
		Slot *slot;
		for (;;)
		{
			slot = m_slots + index;
			if (slot->hash == hash && slot->key == key) {
				*val = slot->value;
				return E_SUCCESS;
			}
			if (Traits::isEmpty(slot->key))
				break;
			index = (index + 1) & mask;
		}
		return -E_NOENT;
	}

	inline int my_strcmp(const char *cs, const char* ct) {
		unsigned char c1;
		unsigned char c2;

		while (1) {
			c1 = *cs++;
			c2 = *ct++;
			if (c1 != c2) 
				return c1 < c2 ? -1 : 1;
			if (!c1)
				break;
		}
		return 0;
	}

	
	inline int Lookup (Session* session, const char* key, Value* val)
    {
		uint hash = Traits::hash(key);
		uint mask = m_numSlots - 1;
		uint index = hash & mask;
		Slot *slot;
		for (;;)
		{
			slot = m_slots + index;
			if (slot->hash == hash && my_strcmp(slot->key.c, key) == 0) {
				*val = slot->value;
				return E_SUCCESS;
			}
			if (Traits::isEmpty(slot->key))
				break;
			index = (index + 1) & mask;
		}
		return -E_NOENT;
	}


	int Insert (Session* session, const Key &key, Value val)
    {
		uint hash = Traits::hash(key);
		for (;;)
		{
			uint mask = m_numSlots - 1;
	        uint index = hash & mask;
			Slot *slot;
			for (;;)
			{
				slot = m_slots + index;
				if (slot->hash == hash && slot->key == key) {
					slot->value = val;
					return E_SUCCESS;
				}
				if (Traits::isEmpty(slot->key))
					break;
				index = (index + 1) & mask;
			}
			slot->hash = hash;
			slot->key = key;
			m_numKeys++;
			if (m_numKeys < m_limitKeys) {
				slot->value = val;
				return E_SUCCESS;
			}
			if (m_numSlots == INITIAL_NUM_SLOTS) {
				Resize(session, 1024);
			} else {
				Resize(session, m_numSlots * 2);
			}
			// Now that everything has relocated, loop, find and return the new slot
		}
	}
	
	int Remove (Session* session, const Key &key)
    {
		return E_SUCCESS;
	}

	void Resize(Session* session, uint newSize)
	{
		void* ptr;
		uint newMask = newSize - 1;
		assert((newSize & newMask) == 0);	// must be power of 2
		if (session->salloc()->AllocateExtent(session, sizeof(Slot) * newSize, 0, &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		Slot *newSlots = (Slot*) ptr;
		Slot *oldSlot = m_slots;
		for (uint i = 0; i < m_numSlots; i++, oldSlot++)
		{
			uint newIndex = oldSlot->hash & newMask;
			Slot *newSlot;
			for (;;)
			{
				newSlot = newSlots + newIndex;
				if (Traits::isEmpty(newSlot->key))
					break;
				newIndex = (newIndex + 1) & newMask;
			}
			*newSlot = *oldSlot;
		}
		m_numSlots = newSize;
		m_slots = newSlots;
		m_limitKeys = (uint) (newSize * m_threshold);
		assert(m_numKeys < m_limitKeys);
	}
};

#endif // __STAMNOS_OSD_COMMON_OPENHASHTABLE_H 
