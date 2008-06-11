// ==================================================================
// @(#)net_Link.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/03/2006
// $Id: net_Link.h,v 1.4 2008-06-11 15:21:47 bqu Exp $
// ==================================================================

#ifndef __JNI_NET_LINK_H__
#define __JNI_NET_LINK_H__

#include <jni_md.h>
#include <jni.h>

#include <net/link.h>
#include <jni/headers/be_ac_ucl_ingi_cbgp_net_Link.h>

// -----[ cbgp_jni_new_net_Interface ]-------------------------------
jobject cbgp_jni_new_net_Interface(JNIEnv * jEnv, jobject joCBGP,
				   net_iface_t * iface);

#endif /* __JNI_NET_LINK_H__ */
