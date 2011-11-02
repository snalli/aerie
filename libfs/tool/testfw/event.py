import sysv_ipc

q = sysv_ipc.MessageQueue(42, sysv_ipc.IPC_CREAT)
s = sysv_ipc.Semaphore(43, sysv_ipc.IPC_CREAT)

m = q.receive(True, 1)
s.release()
print m
q.remove()
s.remove()
