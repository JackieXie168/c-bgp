// ==================================================================
// @(#)bgp_Domain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// $Id: bgp_Domain.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __JNI_BGP_DOMAIN_H__
#define __JNI_BGP_DOMAIN_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Domain.h>

#include <bgp/domain.h>

// -----[ cbgp_jni_new_bgp_Domain ]------------------------------------
jobject cbgp_jni_new_bgp_Domain(JNIEnv * jEnv, jobject joCBGP,
				bgp_domain_t * domain);

#endif /* __JNI_BGP_DOMAIN_H__ */
