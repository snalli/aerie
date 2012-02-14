#ifndef __STAMNOS_DPO_REGISTRY_PROTOCOL_H
#define __STAMNOS_DPO_REGISTRY_PROTOCOL_H

namespace dpo {

class RegistryProtocol {
public:
	
	enum RpcNumbers {
		kLookup = 90,
		kAdd,
		kRemove
	};
};

} // namespace dpo

#endif // __STAMNOS_DPO_REGISTRY_PROTOCOL_H
