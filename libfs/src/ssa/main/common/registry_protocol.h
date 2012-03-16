#ifndef __STAMNOS_SSA_REGISTRY_PROTOCOL_H
#define __STAMNOS_SSA_REGISTRY_PROTOCOL_H

namespace ssa {

class RegistryProtocol {
public:
	
	enum RpcNumbers {
		kLookup = 90,
		kAdd,
		kRemove
	};
};

} // namespace ssa

#endif // __STAMNOS_SSA_REGISTRY_PROTOCOL_H
