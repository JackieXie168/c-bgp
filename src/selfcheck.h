// ==================================================================
// @(#)selfcheck.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 03/04/08
// $Id: selfcheck.h,v 1.1 2008-06-12 14:23:51 bqu Exp $
// ==================================================================

#ifndef __CBGP_SELFCHECK_H__
#define __CBGP_SELFCHECK_H__

#ifdef CYGWIN
# define CBGP_EXP_DECL __declspec(dllexport)
#else
# define CBGP_EXP_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ libcbgp_selfcheck ]--------------------------------------
  CBGP_EXP_DECL int libcbgp_selfcheck();

#ifdef __cplusplus
}
#endif

#endif /* __CBGP_SELFCHECK_H__ */
