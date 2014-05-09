// ==================================================================
// @(#)bgp_Router.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// $Id: bgp_Router.h,v 1.3 2009-03-25 07:49:45 bqu Exp $
// ==================================================================

#ifndef __JNI_BGP_ROUTER_H__
#define __JNI_BGP_ROUTER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Router.h>

#include <bgp/types.h>

// -----[ cbgp_jni_new_bgp_Router ]------------------------------------
jobject cbgp_jni_new_bgp_Router(JNIEnv * jEnv, jobject joCBGP,
				bgp_router_t * router);

#endif /* __JNI_BGP_ROUTER_H__ */
