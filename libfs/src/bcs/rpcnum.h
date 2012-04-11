#ifndef __STAMNOS_IPC_RPC_NUMBERS_H
#define __STAMNOS_IPC_RPC_NUMBERS_H

#define ALL_RPC_NUMBER(module, protocol, handle)                               \
    module##_##protocol##_##handle,

#define RPC_NUMBER(module, protocol, handle)                                   \
    handle = StamnosGlobalRpcNumbers::module##_##protocol##_##handle,

#define DEFINE_RPC_NUMBER(ACTION)                                              \
    ACTION(RPC_NUMBER)


// Module/protocol RPC numbers

#define BCS_IPC_PROTOCOL(ACTION)                                               \
	ACTION(bcs, IpcProtocol, kRpcServerIsAlive)                                \
	ACTION(bcs, IpcProtocol, kRpcSubscribe)

#define BCS_SHARED_BUFFER_PROTOCOL(ACTION)                                     \
	ACTION(bcs, SharedBufferProtocol, kConsume)                                

#define OSD_LOCK_PROTOCOL(ACTION)                                              \
	ACTION(osd, lock_protocol, acquire)                                        \
	ACTION(osd, lock_protocol, acquirev)                                       \
	ACTION(osd, lock_protocol, release)                                        \
	ACTION(osd, lock_protocol, convert)                                        \
	ACTION(osd, lock_protocol, stat)

#define OSD_RLOCK_PROTOCOL(ACTION)                                             \
	ACTION(osd, rlock_protocol, revoke)                                        \
	ACTION(osd, rlock_protocol, retry)

#define OSD_STORAGE_PROTOCOL(ACTION)                                           \
	ACTION(osd, StorageProtocol, kAllocateContainerVector)                     \
	ACTION(osd, StorageProtocol, kAllocateContainer)                           \
	ACTION(osd, StorageProtocol, kAllocateContainerSet)

#define OSD_REGISTRY_PROTOCOL(ACTION)                                          \
	ACTION(osd, RegistryProtocol, kLookup)                                     \
	ACTION(osd, RegistryProtocol, kAdd)                                        \
	ACTION(osd, RegistryProtocol, kRemove)

#define OSD_PUBLISHER_PROTOCOL(ACTION)                                         \
	ACTION(osd, PublisherProtocol, kPublish)                                    

#define OSD_STORAGESYSTEM_PROTOCOL(ACTION)                                     \
	ACTION(osd, StorageSystemProtocol, kMount)

#define PXFS_FILESYSTEM_PROTOCOL(ACTION)                                       \
	ACTION(pxfs, FileSystemProtocol, kMount)


class StamnosGlobalRpcNumbers {
public:

	enum {
		null_rpc = 0x4000,
		BCS_IPC_PROTOCOL(ALL_RPC_NUMBER)
		BCS_SHARED_BUFFER_PROTOCOL(ALL_RPC_NUMBER)
		OSD_LOCK_PROTOCOL(ALL_RPC_NUMBER)
		OSD_RLOCK_PROTOCOL(ALL_RPC_NUMBER)
		OSD_STORAGE_PROTOCOL(ALL_RPC_NUMBER)
		OSD_REGISTRY_PROTOCOL(ALL_RPC_NUMBER)
		OSD_PUBLISHER_PROTOCOL(ALL_RPC_NUMBER)
		OSD_STORAGESYSTEM_PROTOCOL(ALL_RPC_NUMBER)
		PXFS_FILESYSTEM_PROTOCOL(ALL_RPC_NUMBER)
	};

};


#endif // __STAMNOS_IPC_RPC_NUMBERS_H
