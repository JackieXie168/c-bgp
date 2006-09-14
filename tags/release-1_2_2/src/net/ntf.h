// ==================================================================
// @(#)ntf.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/03/2005
// @lastdate 24/03/2005
// ==================================================================

#ifndef __NTF_H__
#define __NTF_H__

// ----- Error codes ----
#define NTF_SUCCESS               0
#define NTF_ERROR_UNEXPECTED      -1
#define NTF_ERROR_OPEN            -2
#define NTF_ERROR_NUM_PARAMS      -3
#define NTF_ERROR_INVALID_ADDRESS -4
#define NTF_ERROR_INVALID_WEIGHT  -5
#define NTF_ERROR_INVALID_DELAY   -6

// -----[ ntf_load ]-------------------------------------------------
extern int ntf_load(char * pcFileName);
// -----[ ntf_save ]-------------------------------------------------
extern int ntf_save(char * pcFileName);

#endif /* __NTF_H__ */
