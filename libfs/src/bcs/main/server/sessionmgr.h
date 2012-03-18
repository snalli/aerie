/**
 * \file sessionmgr.h
 *
 * \brief Bottom-layer session manager
 */

#ifndef __STAMNOS_BCS_SERVER_SESSION_MANAGER_H
#define __STAMNOS_BCS_SERVER_SESSION_MANAGER_H

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/errno.h"


namespace server {

class BaseSession; // forward declaration


/**
 * \brief Manages per client session state 
 *
 * SESSION OBJECT
 * We keep a session per client per storage system.
 *
 * SESSION OBJECT STRUCTURE
 * Each layer encapsulates its per client session state into its Session 
 * data structure which inherits Session structure of the layer below. 
 * Layers must be layered strictly in a single-inheritance structure. 
 * Multiple inheritance is prohibited because we would then need to rely
 * on virtual inheritance to avoid diamonds, which is undesirable as it 
 * has a performance hit.
 * Part of the problem comes in that we want a generic session manager 
 * that could be used as a building block for different storage systems.
 * Alternatively we could have all layers use the same Session class, 
 * which is defined in a source file outside from the layers. 
 * This has several drawbacks with the most important being that 
 * we can't use the same source tree to build multiple storage systems
 *
 * SESSION OBJECT CONSTRUCTION
 * The Session object is created hierarchically starting at the top layer.
 * Each layer has knowledge of the layer below so it can call the layer
 * to construct the rest of the Session Object. 
 *
 * BOTTOM- AND TOP-LAYER SESSION MANAGER 
 * To enable construction at the top layer but be able to find the Session
 * object associated with each client from lower layers we separate the 
 * implementation of the session manager into a bottom-layer session manager 
 * that lives in the BCS layer (bottom layer) and a top-layer session manager 
 * that lives in the file system layer (top layer). The bottom-layer is 
 * responsible for mapping clients to session objects so that layers above 
 * can query it. The top-layer is responsible for creating and destroying 
 * Session Objects as it is the layer that implements the storage system 
 * and therefore has knowledge of when sessions are created/destroyed. 
 */
class BaseSessionManager {
	typedef google::dense_hash_map<int, BaseSession*> ClientSessionMap;
public:
	inline BaseSessionManager();
	inline int Lookup(int clt, BaseSession** session);
	inline int Insert(int clt, BaseSession* session);
	inline int Remove(int clt);

private:
	ClientSessionMap clt_session_map_;
};


BaseSessionManager::BaseSessionManager()
{
	clt_session_map_.set_empty_key(-1);
}


int 
BaseSessionManager::Lookup(int clt, BaseSession** session) 
{
	ClientSessionMap::iterator it;

	if ((it=clt_session_map_.find(clt)) == clt_session_map_.end()) {
		return -E_NOENT;
	}
	*session = it->second;
	return E_SUCCESS;
}


int
BaseSessionManager::Insert(int clt, BaseSession* session) 
{
	clt_session_map_[clt] = session;
	return E_SUCCESS;
}


int
BaseSessionManager::Remove(int clt) 
{
	ClientSessionMap::iterator it;

	if ((it=clt_session_map_.find(clt)) == clt_session_map_.end()) {
		return -E_NOENT;
	}
	clt_session_map_.erase(clt);
	return E_SUCCESS;
}


} // namespace server

#endif // __STAMNOS_BCS_SERVER_SESSION_MANAGER_H
