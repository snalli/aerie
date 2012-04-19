#ifndef __STAMNOS_OSD_SERVER_JOURNAL_H
#define __STAMNOS_OSD_SERVER_JOURNAL_H


/**
 *  TODO Currently the journal provides just a performance model.
 *       Our prototype does not implement recovery.
 */

namespace osd {
namespace server {

class Journal {
public:
	Journal()
	{ }

	int TransactionBegin(int id = 0);
	int TransactionCommit();

	template<typename T>
	void Store(volatile T* addr, T val)
	{
		//*addr = val;
	}

private:
};


} // namespace server
} // namespace osd

#endif // __STAMNOS_OSD_SERVER_JOURNAL_H
