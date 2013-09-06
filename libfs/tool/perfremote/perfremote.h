#include "monitor.h"

#define PERF_ATTACH(monitor)                              \
  Monitor::Client* __monitor_client;                      \
  if (monitor) {                                          \
    __monitor_client = new Monitor::Client(MONITOR_PORT); \
    __monitor_client->attach();                           \
  }

#define PERF_DEATTACH(monitor)                            \
  if (monitor) {                                          \
    __monitor_client->deattach();                         \
  }
