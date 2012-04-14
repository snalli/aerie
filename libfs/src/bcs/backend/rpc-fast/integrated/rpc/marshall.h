#ifndef marshall_h
#define marshall_h

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define max(a,b) ((a>b)?a:b)

struct req_header {
  req_header(unsigned int cid=0, int p=0): proc(p),client_id(cid){}
  int proc;
  unsigned int client_id;
};

struct reply_header {
  reply_header(unsigned int cid=0, int r=0): ret(r),client_id(cid){}
  int ret;
  unsigned int client_id;
};

typedef uint64_t rpc_checksum_t;
typedef int rpc_sz_t;

enum {
	//size of initial buffer allocation 
	DEFAULT_RPC_SZ = 1024,
#if RPC_CHECKSUMMING
	//size of rpc_header includes a 4-byte int to be filled by tcpchan and uint64_t checksum
	RPC_HEADER_SZ = max(sizeof(req_header), sizeof(reply_header)) + sizeof(rpc_sz_t) + sizeof(rpc_checksum_t)
#else
		RPC_HEADER_SZ = max(sizeof(req_header), sizeof(reply_header)) + sizeof(rpc_sz_t)
#endif
};

class marshall {
	private:
		char *_buf;     // Base of the raw bytes buffer (dynamically readjusted)
		int _capa;      // Capacity of the buffer
		int _ind;       // Read/write head position

	public:
		marshall() {
			_buf = (char *) malloc(sizeof(char)*DEFAULT_RPC_SZ);
			assert(_buf);
			_capa = DEFAULT_RPC_SZ;
			_ind = RPC_HEADER_SZ;
		}

		~marshall() { 
			if (_buf) 
				free(_buf); 
		}

		int size() { return _ind;}
		char *cstr() { return _buf;}

		void rawbyte(unsigned char);
		void rawbytes(const char *, int);

		// Return the current contents (including header) as a string
		const std::string str() const {
			std::string tmps = std::string(_buf,_ind);
			return tmps;
		}

		void pack(int i);

		void pack_req_header(const req_header &h) {
			int saved_sz = _ind;
			//leave the first 4-byte empty for channel to fill size of pdu
			_ind = sizeof(rpc_sz_t); 
#if RPC_CHECKSUMMING
			_ind += sizeof(rpc_checksum_t);
#endif
			pack(h.proc);
			pack((int) h.client_id);
			_ind = saved_sz;
		}

		void pack_reply_header(const reply_header &h) {
			int saved_sz = _ind;
			//leave the first 4-byte empty for channel to fill size of pdu
			_ind = sizeof(rpc_sz_t); 
#if RPC_CHECKSUMMING
			_ind += sizeof(rpc_checksum_t);
#endif
			pack(h.ret);
			pack((int) h.client_id);
			_ind = saved_sz;
		}

		void take_buf(char **b, int *s) {
			*b = _buf;
			*s = _ind;
			_buf = NULL;
			_ind = 0;
			return;
		}
};
marshall& operator<<(marshall &, unsigned int);
marshall& operator<<(marshall &, int);
marshall& operator<<(marshall &, unsigned char);
marshall& operator<<(marshall &, char);
marshall& operator<<(marshall &, unsigned short);
marshall& operator<<(marshall &, short);
marshall& operator<<(marshall &, unsigned long long);
marshall& operator<<(marshall &, const std::string &);

class unmarshall {
	private:
		char *_buf;
		int _sz;
		int _ind;
		bool _ok;
	public:
		unmarshall(): _buf(NULL),_sz(0),_ind(0),_ok(false) {}
		unmarshall(char *b, int sz): _buf(b),_sz(sz),_ind(0),_ok(true) {}
		~unmarshall() {
		  //if (_buf) free(_buf);
		}
		//take contents from another unmarshall object
		void take_in(unmarshall &another);
		bool ok() { return _ok; }
		char *cstr() { return _buf;}
		bool okdone();
		unsigned int rawbyte();
		void rawbytes(std::string &s, unsigned int n);

		int ind() { return _ind;}
		int size() { return _sz;}
		void unpack(int *); //non-const ref
		void take_buf(char **b, int *sz) {
			*b = _buf;
			*sz = _sz;
			_sz = _ind = 0;
			_buf = NULL;
		}

		void unpack_req_header(req_header *h) {
			//the first 4-byte is for channel to fill size of pdu
			_ind = sizeof(rpc_sz_t); 
#if RPC_CHECKSUMMING
			_ind += sizeof(rpc_checksum_t);
#endif
			unpack(&h->proc);
			unpack((int*) &h->client_id);
			_ind = RPC_HEADER_SZ;
		}

		void unpack_reply_header(reply_header *h) {
			//the first 4-byte is for channel to fill size of pdu
			_ind = sizeof(rpc_sz_t); 
#if RPC_CHECKSUMMING
			_ind += sizeof(rpc_checksum_t);
#endif
			unpack(&h->ret);
			unpack((int*) &h->client_id);
			_ind = RPC_HEADER_SZ;
		}
};

unmarshall& operator>>(unmarshall &, unsigned char &);
unmarshall& operator>>(unmarshall &, char &);
unmarshall& operator>>(unmarshall &, unsigned short &);
unmarshall& operator>>(unmarshall &, short &);
unmarshall& operator>>(unmarshall &, unsigned int &);
unmarshall& operator>>(unmarshall &, int &);
unmarshall& operator>>(unmarshall &, unsigned long long &);
unmarshall& operator>>(unmarshall &, std::string &);

template <class C> marshall &
operator<<(marshall &m, std::vector<C> v)
{
	m << (unsigned int) v.size();
	for(unsigned i = 0; i < v.size(); i++)
		m << v[i];
	return m;
}

template <class C> unmarshall &
operator>>(unmarshall &u, std::vector<C> &v)
{
	unsigned n;
	u >> n;
	for(unsigned i = 0; i < n; i++){
		C z;
		u >> z;
		v.push_back(z);
	}
	return u;
}

template <class A, class B> marshall &
operator<<(marshall &m, const std::map<A,B> &d) {
	typename std::map<A,B>::const_iterator i;

	m << (unsigned int) d.size();

	for (i = d.begin(); i != d.end(); i++) {
		m << i->first << i->second;
	}
	return m;
}

template <class A, class B> unmarshall &
operator>>(unmarshall &u, std::map<A,B> &d) {
	unsigned int n;
	u >> n;

	d.clear();

	for (unsigned int lcv = 0; lcv < n; lcv++) {
		A a;
		B b;
		u >> a >> b;
		d[a] = b;
	}
	return u;
}

#endif
