// ==================================================================
// @(#)bgp_Peer.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/04/2006
// @lastdate 21/04/2006
// ==================================================================

#ifndef __JNI_BGP_PEER_H__
#define __JNI_BGP_PEER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Peer.h>

// -----[ cbgp_jni_new_bgp_Peer ]------------------------------------
jobject cbgp_jni_new_bgp_Peer(JNIEnv * jEnv, jobject joCBGP,
			      SBGPPeer * pPeer);

#endif /* __JNI_BGP_PEER_H__ */
