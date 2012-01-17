#include "mfs/client/sb_factory.h"
#include "client/session.h"
#include "dpo/containers/super/container.h"
#include "mfs/client/sb.h"

namespace mfs {
namespace client {


SuperBlockFactory::SuperBlockFactory()
{
	
}


int
SuperBlockFactory::Make(::client::Session* session, ::client::SuperBlock** sbp)
{
	SuperBlock* sb; 
	dpo::containers::client::SuperContainer::Object* obj;

	sb = new SuperBlock();

	//if ((obj = dpo::containers::client::SuperContainer::Object::Make(session)) == NULL)
	
	*sbp = sb;
	return E_SUCCESS;
}


int 
SuperBlockFactory::Load()
{


}


} // namespace client
} // namespace mfs
