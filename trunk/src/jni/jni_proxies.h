// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 12/10/2007
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
  void * jni_proxy_lookup2(JNIEnv * jEnv, jobject joObject,
			   char * pcLine, int iLine);
  // -----[ jni_proxy_get ]------------------------------------------
  jobject jni_proxy_get2(JNIEnv * jEnv, void * pObject,
			char * pcLine, int iLine);

  // -----[ jni_proxy_get_CBGP ]-------------------------------------
  jobject jni_proxy_get_CBGP(JNIEnv * jEnv, jobject joObject);

  // -----[ jni_lock ]-----------------------------------------------
  void jni_lock(JNIEnv * jEnv);
  // -----[ jni_unlock ]---------------------------------------------
  void jni_unlock(JNIEnv * jEnv);

  // -----[ _jni_proxies_init ]--------------------------------------
  void _jni_proxies_init();
  // -----[ _jni_proxies_destroy ]-----------------------------------
  void _jni_proxies_destroy(JNIEnv * jEnv);
  // -----[ _jni_proxies_invalidate ]--------------------------------
  void _jni_proxies_invalidate(JNIEnv * jEnv);

  // -----[ _jni_lock_init ]------------------------------------------
  void _jni_lock_init(JNIEnv * jEnv);
  // -----[ _jni_lock_destroy ]---------------------------------------
  void _jni_lock_destroy(JNIEnv * jEnv);

#define return_jni_unlock(E, R) if (1) { jni_unlock(E); return R; }
#define return_jni_unlock2(E) if (1) { jni_unlock(E); return; }
#define jni_proxy_lookup(E,O) jni_proxy_lookup2(E,O,__FILE__,__LINE__)
#define jni_proxy_get(E,O) jni_proxy_get2(E,O,__FILE__,__LINE__)

#ifdef __cplusplus
}
#endif

#endif /* __JNI_PROXIES_H__ */
