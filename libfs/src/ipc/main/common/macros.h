#ifndef __STAMNOS_IPC_COMMON_MACROS_H
#define __STAMNOS_IPC_COMMON_MACROS_H


#define RPC_REGISTER_HANDLER(rpcs)                                                  \
	/* register a handler  */                                                       \
	template<class S, class A1, class R>                                            \
		void reg(unsigned int proc, S* sob, int (S::*meth)(const A1 a1, R & r))     \
	{                                                                               \
		rpcs->reg(proc, sob, meth);                                                 \
	}                                                                               \
                                                                                    \
	template<class S, class A1, class A2, class R>                                  \
		void reg(unsigned int proc, S* sob, int (S::*meth)(const A1 a1, const A2,   \
					R & r))                                                         \
	{                                                                               \
		rpcs->reg(proc, sob, meth);                                                 \
	}                                                                               \
                                                                                    \
	template<class S, class A1, class A2, class A3, class R>                        \
		void reg(unsigned int proc, S* sob, int (S::*meth)(const A1, const A2,      \
					const A3, R & r))                                               \
	{                                                                               \
		rpcs->reg(proc, sob, meth);                                                 \
	}                                                                               \
                                                                                    \
	template<class S, class A1, class A2, class A3, class A4, class R>              \
		void reg(unsigned int proc, S* sob, int (S::*meth)(const A1, const A2,      \
					const A3, const A4, R & r))                                     \
	{                                                                               \
        rpcs->reg(proc, sob, meth);                                                 \
	}                                                                               \
                                                                                    \
	template<class S, class A1, class A2, class A3, class A4, class A5, class R>    \
		void reg(unsigned int proc, S* sob, int (S::*meth)(const A1, const A2,      \
					const A3, const A4, const A5,                                   \
					R & r))                                                         \
	{                                                                               \
		rpcs->reg(proc, sob, meth);                                                 \
	}                                                                               \
                                                                                    \
	template<class S, class A1, class A2, class A3, class A4, class A5, class A6,   \
		class R>                                                                    \
			void reg(unsigned int proc, S* sob, int (S::*meth)(const A1, const A2,  \
						const A3, const A4, const A5,                               \
						const A6, R & r))                                           \
    {                                                                               \
	    rpcs->reg(proc, sob, meth);                                                 \
    }                                                                               \
                                                                                    \
	template<class S, class A1, class A2, class A3, class A4, class A5, class A6,   \
		class A7, class R>                                                          \
			void reg(unsigned int proc, S* sob, int (S::*meth)(const A1, const A2,  \
						const A3, const A4, const A5,                               \
						const A6, const A7,                                         \
						R & r))                                                     \
    {                                                                               \
        rpcs->reg(proc, sob, meth);                                                 \
	}


#define RPC_CALL(RPCC, to_max)                                                      \
    typedef rpcc::TO TO;                                                            \
                                                                                    \
    template<class R>                                                               \
        int call(unsigned int proc, R & r, TO to = to_max)                          \
    {                                                                               \
	    return RPCC->call(proc, r, to);                                             \
	}                                                                               \
                                                                                    \
    template<class R, class A1>                                                     \
        int call(unsigned int proc, const A1 & a1, R & r, TO to = to_max)           \
    {                                                                               \
	    return RPCC->call(proc, a1, r, to);                                         \
	}                                                                               \
                                                                                    \
    template<class R, class A1, class A2>                                           \
        int call(unsigned int proc, const A1 & a1, const A2 & a2, R & r,            \
                 TO to = to_max)                                                    \
    {                                                                               \
	    return RPCC->call(proc, a1, a2, r, to);                                     \
	}                                                                               \
                                                                                    \
    template<class R, class A1, class A2, class A3>                                 \
        int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3,    \
                 R & r, TO to = to_max)                                             \
    {                                                                               \
		return RPCC->call(proc, a1, a2, a3, r, to);                                 \
	}                                                                               \
                                                                                    \
    template<class R, class A1, class A2, class A3, class A4>                       \
	    int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3,    \
                 const A4 & a4, R & r, TO to = to_max)                              \
    {                                                                               \
	    return RPCC->call(proc, a1, a2, a3, a4, r, to);                             \
	}                                                                               \
                                                                                    \
    template<class R, class A1, class A2, class A3, class A4, class A5>             \
        int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3,    \
                 const A4 & a4, const A5 & a5, R & r, TO to = to_max)               \
    {                                                                               \
	    return RPCC->call(proc, a1, a2, a3, a4, a5, r, to);                         \
	}                                                                               \
                                                                                    \
    template<class R, class A1, class A2, class A3, class A4, class A5,             \
             class A6>                                                              \
        int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3,    \
                 const A4 & a4, const A5 & a5, const A6 & a6,                       \
                 R & r, TO to = to_max)                                             \
    {                                                                               \
	    return RPCC->call(proc, a1, a2, a3, a4, a5, a6, r, to);                     \
	}                                                                               \
                                                                                    \
    template<class R, class A1, class A2, class A3, class A4, class A5,             \
             class A6, class A7>                                                    \
        int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3,    \
                 const A4 & a4, const A5 & a5, const A6 &a6, const A7 &a7,          \
                 R & r, TO to = to_max)                                             \
    {                                                                               \
	    return RPCC->call(proc, a1, a2, a3, a4, a5, a6, a7, r, to);                 \
	}                                                                               



#endif // __STAMNOS_IPC_COMMON_RPC_H
