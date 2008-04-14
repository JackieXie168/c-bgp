// ==================================================================
// @(#)ntf.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/03/2005
// $Id: ntf.h,v 1.3 2008-04-14 09:16:04 bqu Exp $
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

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ntf_load ]-----------------------------------------------
  int ntf_load(const char * file_name);
  // -----[ ntf_save ]-----------------------------------------------
  int ntf_save(const char * file_name);

#ifdef __cplusplus
}
#endif

#endif /* __NTF_H__ */
