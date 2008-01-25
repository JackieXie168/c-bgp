// ==================================================================
// @(#)jni_util.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// @lastdate 09/01/2008
// ==================================================================

#ifndef __JNI_UTIL_H__
#define __JNI_UTIL_H__

#include <bgp/as_t.h>
#include <bgp/comm_t.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/net_types.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/routing.h>

#include <jni.h>

// -----[ context-structure for building vectors of links/routes ]---
typedef struct {
  union {
    SNetNode * pNode;
    SBGPRouter * pRouter;
    jobject joHashtable;
  };
  jobject joVector;
  jobject joCBGP;
  JNIEnv * jEnv;
} SJNIContext;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cbgp_jni_new_ConsoleEvent ]------------------------------
  jobject cbgp_jni_new_ConsoleEvent(JNIEnv * env, char * pcMessage);
  // -----[ cbgp_jni_Communities_append ]----------------------------
  int cbgp_jni_Communities_append(JNIEnv * env, jobject joCommunities,
				  comm_t tCommunity);
  // -----[ cbgp_jni_new_ASPath ]------------------------------------
  jobject cbgp_jni_new_ASPath(JNIEnv * env, SBGPPath * pPath);
  // -----[ cbgp_jni_ASPath_append ]---------------------------------
  int cbgp_jni_ASPath_append(JNIEnv * env, jobject joASPath,
			     SPathSegment * pSegment);
  // -----[ cbgp_jni_new_ASPathSegment ]-----------------------------
  jobject cbgp_jni_new_ASPathSegment(JNIEnv * env,
				     SPathSegment * pSegment);
  // -----[ cbgp_jni_ASPathSegment_append ]--------------------------
  int cbgp_jni_ASPathSegment_append(JNIEnv * env, jobject joASPathSeg,
				    uint16_t uAS);
  // -----[ cbgp_jni_new_IPPrefix ]----------------------------------
  jobject cbgp_jni_new_IPPrefix(JNIEnv * env, SPrefix sPrefix);
  // -----[ cbgp_jni_new_IPAddress ]---------------------------------
  jobject cbgp_jni_new_IPAddress(JNIEnv * env, net_addr_t tAddr);
  // -----[ cbgp_jni_new_IPRoute ]-----------------------------------
  jobject cbgp_jni_new_IPRoute(JNIEnv * env, SPrefix sPrefix,
			       SNetRouteInfo * pRoute);
  // -----[ cbgp_jni_new_BGPRoute ]----------------------------------
  jobject cbgp_jni_new_BGPRoute(JNIEnv * jEnv, SRoute * pRoute,
				jobject joHashtable);
  // -----[ cbgp_jni_new_BGPMessage ]----------------------------------
  jobject cbgp_jni_new_BGPMessage(JNIEnv * jEnv, SNetMessage * pMessage);
  // -----[ ip_jstring_to_address ]----------------------------------
  int ip_jstring_to_address(JNIEnv * env, jstring jsAddr, net_addr_t * ptAddr);
  // -----[ ip_jstring_to_prefix ]-----------------------------------
  int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix, SPrefix * psPrefix);
  // -----[ ip_jstring_to_dest ]-------------------------------------
  int ip_jstring_to_dest(JNIEnv * jEnv, jstring jsDest, SNetDest * pDest);
  // -----[ cbgp_jni_net_node_from_string ]--------------------------
  SNetNode * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr);
  // -----[ cbgp_jni_net_link_from_string ]--------------------------
  SNetLink * cbgp_jni_net_link_from_string(JNIEnv * env, jstring jsSrcAddr,
					   jstring jsDstAddr);
  // -----[ cbgp_jni_bgp_router_from_string ]------------------------
  SBGPRouter * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr);
  // -----[ cbgp_jni_bgp_peer_from_string ]--------------------------
  SBGPPeer * cbgp_jni_bgp_peer_from_string(JNIEnv * env,
					   jstring jsRouterAddr,
					   jstring jsPeerAddr);
  // -----[ cbgp_jni_net_domain_from_int ]---------------------------
  SIGPDomain * cbgp_jni_net_domain_from_int(JNIEnv * env, jint iNumber);
  // -----[ cbgp_jni_bgp_domain_from_int ]---------------------------
  SBGPDomain * cbgp_jni_bgp_domain_from_int(JNIEnv * env, jint iNumber);

  // -----[ cbgp_jni_net_error_str ]---------------------------------
  jstring cbgp_jni_net_error_str(JNIEnv * jEnv, int iErrorCode);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_UTIL_H__ */
