// ==================================================================
// @(#)net_IGPDomain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// $Id: net_IGPDomain.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __JNI_NET_IGPDOMAIN_H__
#define __JNI_NET_IGPDOMAIN_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_net_IGPDomain.h>

#include <net/igp_domain.h>

// -----[ cbgp_jni_new_net_IGPDomain ]-------------------------------
jobject cbgp_jni_new_net_IGPDomain(JNIEnv * jEnv, jobject joCBGP,
				   igp_domain_t * domain);

#endif /* __JNI_NET_IGPDOMAIN_H__ */
