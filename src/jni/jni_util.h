// ==================================================================
// @(#)jni_util.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 21/03/2006
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
  };
  jobject joVector;
  JNIEnv * jEnv;
} SJNIContext;

// -----[ cbgp_jni_throw_CBGPException ]-----------------------------
extern void cbgp_jni_throw_CBGPException(JNIEnv * env, char * pcMsg);
// -----[ cbgp_jni_new_ConsoleEvent ]--------------------------------
extern jobject cbgp_jni_new_ConsoleEvent(JNIEnv * env, char * pcMessage);
// -----[ cbgp_jni_Communities_append ]------------------------------
extern int cbgp_jni_Communities_append(JNIEnv * env, jobject joCommunities,
				       comm_t tCommunity);
// -----[ cbgp_jni_new_ASPath ]--------------------------------------
extern jobject cbgp_jni_new_ASPath(JNIEnv * env, SBGPPath * pPath);
// -----[ cbgp_jni_ASPath_append ]-----------------------------------
extern int cbgp_jni_ASPath_append(JNIEnv * env, jobject joASPath,
				  SPathSegment * pSegment);
// -----[ cbgp_jni_new_ASPathSegment ]-------------------------------
extern jobject cbgp_jni_new_ASPathSegment(JNIEnv * env,
					  SPathSegment * pSegment);
// -----[ cbgp_jni_ASPathSegment_append ]----------------------------
extern int cbgp_jni_ASPathSegment_append(JNIEnv * env, jobject joASPathSeg,
					 uint16_t uAS);
// -----[ cbgp_jni_new_IPPrefix ]------------------------------------
extern jobject cbgp_jni_new_IPPrefix(JNIEnv * env, SPrefix sPrefix);
// -----[ cbgp_jni_new_IPAddress ]-----------------------------------
extern jobject cbgp_jni_new_IPAddress(JNIEnv * env, net_addr_t tAddr);
// -----[ cbgp_jni_new_IGPDomain ]-----------------------------------
extern jobject cbgp_jni_new_IGPDomain(JNIEnv * env, SIGPDomain * pDomain);
// -----[ cbgp_jni_new_Node ]----------------------------------------
extern jobject cbgp_jni_new_Node(JNIEnv * env, SNetNode * pNode);
// -----[ cbgp_jni_new_Link ]----------------------------------------
extern jobject cbgp_jni_new_Link(JNIEnv * env, SNetLink * pLink);
// -----[ cbgp_jni_new_IPRoute ]-------------------------------------
extern jobject cbgp_jni_new_IPRoute(JNIEnv * env, SPrefix sPrefix,
				    SNetRouteInfo * pRoute);
// -----[ cbgp_jni_new_IPTrace ]-------------------------------------
extern jobject cbgp_jni_new_IPTrace(JNIEnv * env, net_addr_t tSrc,
				    net_addr_t tDst, SNetPath * pPath,
				    int iStatus, net_link_delay_t tDelay,
				    net_link_delay_t tWeight);
// -----[ cbgp_jni_new_BGPDomain ]-----------------------------------
extern jobject cbgp_jni_new_BGPDomain(JNIEnv * env, SBGPDomain * pDomain);
// -----[ cbgp_jni_new_BGPRouter ]-----------------------------------
extern jobject cbgp_jni_new_BGPRouter(JNIEnv * env, SBGPRouter * pRouter);
// -----[ cbgp_jni_new_BGPPeer ]-------------------------------------
extern jobject cbgp_jni_new_BGPPeer(JNIEnv * env, SPeer * pPeer);
// -----[ cbgp_jni_new_BGPRoute ]------------------------------------
extern jobject cbgp_jni_new_BGPRoute(JNIEnv * env, SRoute * pRoute);
// -----[ ip_jstring_to_address ]------------------------------------
extern int ip_jstring_to_address(JNIEnv * env, jstring jsAddr, net_addr_t * ptAddr);
// -----[ ip_jstring_to_prefix ]-------------------------------------
extern int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix, SPrefix * psPrefix);
// -----[ cbgp_jni_net_node_from_string ]----------------------------
extern SNetNode * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr);
// -----[ cbgp_jni_net_link_from_string ]----------------------------
extern SNetLink * cbgp_jni_net_link_from_string(JNIEnv * env, jstring jsSrcAddr,
						jstring jsDstAddr);
// -----[ cbgp_jni_bgp_router_from_string ]--------------------------
extern SBGPRouter * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr);
// -----[ cbgp_jni_bgp_peer_from_string ]----------------------------
extern SBGPPeer * cbgp_jni_bgp_peer_from_string(JNIEnv * env,
						jstring jsRouterAddr,
						jstring jsPeerAddr);
// -----[ cbgp_jni_net_domain_from_int ]-----------------------------
extern SIGPDomain * cbgp_jni_net_domain_from_int(JNIEnv * env,
						 jint iNumber);
// -----[ cbgp_jni_bgp_domain_from_int ]-----------------------------
extern SBGPDomain * cbgp_jni_bgp_domain_from_int(JNIEnv * env,
						 jint iNumber);

#endif /* __JNI_UTIL_H__ */
