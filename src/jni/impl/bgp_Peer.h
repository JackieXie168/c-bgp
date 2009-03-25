// ==================================================================
// @(#)bgp_Peer.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/04/2006
// $Id: bgp_Peer.h,v 1.3 2009-03-25 07:49:45 bqu Exp $
// ==================================================================

#ifndef __JNI_BGP_PEER_H__
#define __JNI_BGP_PEER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Peer.h>

#include <bgp/types.h>

// -----[ cbgp_jni_new_bgp_Peer ]------------------------------------
jobject cbgp_jni_new_bgp_Peer(JNIEnv * jEnv, jobject joCBGP,
			      bgp_peer_t * peer);

#endif /* __JNI_BGP_PEER_H__ */
