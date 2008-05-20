// ==================================================================
// @(#)listener.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/06/2007
// $Id: listener.h,v 1.2 2008-05-20 12:11:38 bqu Exp $
// ==================================================================

#ifndef __JNI_LISTENER_H__
#define __JNI_LISTENER_H__

#include <jni.h>

// -----[ SListenerCtx ]---------------------------------------------
/**
 * Context structure used for Java callbacks (listeners)
 *
 * IMPORTANT NOTE: the joListener field must contain a global
 * reference to the Java listener !!! Use NewGlobalRef to create a
 * global reference from a local reference.
 */
typedef struct {
  JavaVM * jVM;
  jobject joListener;
} SJNIListener;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ jni_listener_init ]--------------------------------------
  void jni_listener_init(SJNIListener * pListener);
  // -----[ jni_listener_set ]---------------------------------------
  void jni_listener_set(SJNIListener * pListener, JNIEnv * jEnv,
			jobject joListener);
  // -----[ jni_listener_unset ]-------------------------------------
  void jni_listener_unset(SJNIListener * pListener, JNIEnv * jEnv);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_LISTENER_H__ */
