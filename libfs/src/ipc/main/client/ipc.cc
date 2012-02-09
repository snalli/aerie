IpcLayer::IpcLayer(

int 
IpcLayer::Init(int principal_id, const char* xdst)
{
	struct sockaddr_in dst; //server's ip address
	int                rport;
	std::ostringstream host;
	const char*        hname;

	principal_id = getuid();
	
	// setup RPC for making calls to the server
	make_sockaddr(xdst, &dst);
	rpc_client = new rpcc(dst);
	assert (rpc_client->bind() == 0);

	// setup RPC for receiving callbacks from the server
	srandom(getpid());
	rport = 20000 + (getpid() % 10000);
	rpc_server = new rpcs(rport);
	hname = "127.0.0.1";
	host << hname << ":" << rport;
	id = host.str();
	std::cout << "Client: id="<<id<<std::endl;

	// contact the server and tell him my rpc address to subscribe 
	// for async rpc response
	if ((r = cl2srv_->call(ipc_protocol::subscribe, cl2srv_->id(), id, unused)) !=
	    0) 
	{
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
		        "failed to subscribe client: %u\n", cl2srv_->id());
	}


}

// factory method
int
IpcLayer::Create() 
{
	IpcLayer* ipc;

	if ((ipc = new IpcLayer() == NULL) {
		return -E_NOMEM;
	}
	return ipc->Init();
}
