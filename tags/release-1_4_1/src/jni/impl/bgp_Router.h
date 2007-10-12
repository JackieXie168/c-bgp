// ==================================================================
// @(#)bgp_Router.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/04/2006
// @lastdate 21/04/2006
// ==================================================================

#ifndef __JNI_BGP_ROUTER_H__
#define __JNI_BGP_ROUTER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Router.h>

#include <bgp/as_t.h>

// -----[ cbgp_jni_new_bgp_Router ]------------------------------------
jobject cbgp_jni_new_bgp_Router(JNIEnv * jEnv, jobject joCBGP,
				SBGPRouter * pRouter);

#endif /* __JNI_BGP_ROUTER_H__ */
