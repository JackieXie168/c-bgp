// ==================================================================
// @(#)jni_util.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 21/02/2005
// ==================================================================

#ifndef __JNI_UTIL_H__
#define __JNI_UTIL_H__

#include <bgp/as_t.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/routing.h>

#include <jni.h>

// -----[ context-structure for building vectors of links/routes ]---
typedef struct {
  union {
    SBGPRouter * pRouter;
    SNetNode * pNode;
  };
  jobject joVector;
  JNIEnv * jEnv;
} SRouteDumpCtx;

// -----[ cbgp_jni_new ]---------------------------------------------
extern jobject cbgp_jni_new(JNIEnv * env, const char * pcClass,
			    const char * pcConstr, ...);
// -----[ cbgp_jni_call_void ]---------------------------------------
extern int cbgp_jni_call_void(JNIEnv * env, jobject joObject,
			      const char * pcMethod,
			      const char * pcSignature, ...);
// -----[ cbgp_jni_throw_CBGPException ]-----------------------------
extern void cbgp_jni_throw_CBGPException(JNIEnv * env, char * pcMsg);
// -----[ cbgp_jni_new_ASPath ]--------------------------------------
extern jobject cbgp_jni_new_ASPath(JNIEnv * env, SPath * pPath);
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
// -----[ cbgp_jni_new_BGPPeer ]-------------------------------------
extern jobject cbgp_jni_new_BGPPeer(JNIEnv * env, SPeer * pPeer);
// -----[ cbgp_jni_new_BGPRoute ]------------------------------------
extern jobject cbgp_jni_new_BGPRoute(JNIEnv * env, SRoute * pRoute);
// -----[ cbgp_jni_new_Vector ]--------------------------------------
extern jobject cbgp_jni_new_Vector(JNIEnv * env);
// -----[ cbgp_jni_Vector_add ]--------------------------------------
extern int cbgp_jni_Vector_add(JNIEnv * env, jobject joVector,
			       jobject joObject);

// -----[ ip_jstring_to_address ]------------------------------------
extern int ip_jstring_to_address(JNIEnv * env, jstring jsAddr, net_addr_t * ptAddr);
// -----[ ip_jstring_to_prefix ]-------------------------------------
extern int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix, SPrefix * psPrefix);
// -----[ cbgp_jni_net_node_from_string ]----------------------------
extern SNetNode * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr);
// -----[ cbgp_jni_bgp_router_from_string ]--------------------------
extern SBGPRouter * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr);

#endif
