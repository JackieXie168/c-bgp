// ==================================================================
// @(#)net_Subnet.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/10/2007
// $Id: net_Subnet.h,v 1.3 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __JNI_NET_SUBNET_H__
#define __JNI_NET_SUBNET_H__

#include <jni_md.h>
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cbgp_jni_new_net_Subnet ]----------------------------------
  jobject cbgp_jni_new_net_Subnet(JNIEnv * jEnv, jobject joCBGP,
				  net_subnet_t * subnet);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_NET_SUBNET_H__ */
