#ifndef __STAMNOS_OSD_COMMON_JOURNAL_H
#define __STAMNOS_OSD_COMMON_JOURNAL_H

namespace osd {
namespace common {

class Journal {
public:
	enum Mode {
		Client = 1,
		Server = 2,
	};
};

} // namespace common
} // namespace osd

#endif // __STAMNOS_OSD_COMMON_JOURNAL_H

