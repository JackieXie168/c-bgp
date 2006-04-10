// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 27/03/2006
// ==================================================================

#ifndef __JNI_PROXIES_H__
#define __JNI_PROXIES_H__

// -----[ jni_proxy_add ]--------------------------------------------
extern void jni_proxy_add(jint jiHashCode, void * pObject);
// -----[ jni_proxy_remove ]-----------------------------------------
extern void jni_proxy_remove(jint jiHashCode);
// -----[ jni_proxy_lookup ]-----------------------------------------
extern void * jni_proxy_lookup(jint jiHashCode);

#endif
