// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 29/06/2007
// ==================================================================

#ifndef __JNI_PROXIES_H__
#define __JNI_PROXIES_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ jni_proxy_add ]------------------------------------------
  void jni_proxy_add(JNIEnv * jEnv, jobject joObject, void * pObject);
  // -----[ jni_proxy_remove ]---------------------------------------
  void jni_proxy_remove(JNIEnv * jEnv, jobject joObject);
  // -----[ jni_proxy_lookup ]---------------------------------------
  void * jni_proxy_lookup(JNIEnv * jEnv, jobject joObject);
  // -----[ jni_proxy_get_CBGP ]-------------------------------------
  jobject jni_proxy_get_CBGP(JNIEnv * jEnv, jobject joObject);

  // -----[ jni_init_lock ]------------------------------------------
  void jni_init_lock(jobject joObj);
  // -----[ jni_lock ]-----------------------------------------------
  void jni_lock(JNIEnv * jEnv);
  // -----[ jni_unlock ]---------------------------------------------
  void jni_unlock(JNIEnv * jEnv);

#define return_jni_unlock(E, R) if (1) { jni_unlock(E); return R; }
#define return_jni_unlock2(E) if (1) { jni_unlock(E); return; }

#ifdef __cplusplus
}
#endif

#endif /* __JNI_PROXIES_H__ */
