int
IpcLayer::Subscribe(int clt, std::string id, int& unused)
{
	sockaddr_in           dstsock;
	rpcc*                 cl;
	lock_protocol::status r = lock_protocol::OK;

	pthread_mutex_lock(mutex_);
	make_sockaddr(id.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if (cl->bind() == 0) {
		clients_[clt] = cl;
	} else {
		printf("failed to bind to clt %d\n", clt);
	}
	pthread_mutex_unlock(mutex_);
	return r;
}
