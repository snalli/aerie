#ifndef __STAMNOS_SPA_CONST_H
#define __STAMNOS_SPA_CONST_H
// insight - Here is kBlockSize
// it is 4096 Bytes or 4K.
// increasing this slows us down
// decreasing it does not allow us to run
// 4K is good !
const int kBlockShift = 12;
const int kBlockSize  = (1 << kBlockShift);

#endif // __STAMNOS_SPA_PERSISTENT_REGION_H
