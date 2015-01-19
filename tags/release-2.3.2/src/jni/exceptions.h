// ==================================================================
// @(#)exceptions.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/12/2007
// $Id: exceptions.h,v 1.2 2009-03-24 15:59:55 bqu Exp $
// ==================================================================

#ifndef __JNI_EXCEPTIONS_H__
#define __JNI_EXCEPTIONS_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ throw_CBGPException ]------------------------------------
  void throw_CBGPException(JNIEnv * jEnv, const char * pcMsg, ...);
  // -----[ throw_CBGPScriptException ]------------------------------
  void throw_CBGPScriptException(JNIEnv * jEnv,
				 const char * pcMsg,
				 const char * pcFileName,
				 int iLineNumber);
  // -----[ throw_InvalidDestination ]-------------------------------
  void throw_InvalidDestination(JNIEnv * jEnv, jstring jsDest);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_EXCEPTIONS_H__ */
