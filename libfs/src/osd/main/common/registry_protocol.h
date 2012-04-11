#ifndef __STAMNOS_OSD_REGISTRY_PROTOCOL_H
#define __STAMNOS_OSD_REGISTRY_PROTOCOL_H

namespace osd {

class RegistryProtocol {
public:
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(OSD_REGISTRY_PROTOCOL)
	};
};

} // namespace osd

#endif // __STAMNOS_OSD_REGISTRY_PROTOCOL_H
