// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 19/04/2006
// ==================================================================

#ifndef __JNI_PROXIES_H__
#define __JNI_PROXIES_H__

#include <jni.h>

// -----[ jni_proxy_add ]--------------------------------------------
extern void jni_proxy_add(jint jiHashCode, void * pObject);
// -----[ jni_proxy_remove ]-----------------------------------------
extern void jni_proxy_remove(jint jiHashCode);
// -----[ jni_proxy_lookup ]-----------------------------------------
extern void * jni_proxy_lookup(JNIEnv * jEnv, jint jiHashCode);
// -----[ jni_proxy_get_CBGP ]---------------------------------------
extern jobject jni_proxy_get_CBGP(JNIEnv * jEnv, jobject joObject);

#endif
