// ==================================================================
// @(#)jni_util.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 11/02/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <bgp/as_t.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/link.h>
#include <net/network.h>
#include <jni.h>

#define CLASS_IPPrefix "be/ac/ucl/ingi/cbgp/IPPrefix"
#define CONSTR_IPPrefix "(BBBBB)V"

#define CLASS_IPAddress "be/ac/ucl/ingi/cbgp/IPAddress"
#define CONSTR_IPAddress "(BBBB)V"

#define CLASS_Link "be/ac/ucl/ingi/cbgp/Link"
#define CONSTR_Link "(Lbe/ac/ucl/ingi/cbgp/IPAddress;JJZ)V"

#define CLASS_IPRoute "be/ac/ucl/ingi/cbgp/IPRoute"
#define CONSTR_IPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;B)V"

#define CLASS_BGPPeer "be/ac/ucl/ingi/cbgp/BGPPeer"
#define CONSTR_BGPPeer "(Lbe/ac/ucl/ingi/cbgp/IPAddress;IB)V"

#define CLASS_BGPRoute "be/ac/ucl/ingi/cbgp/BGPRoute"
#define CONSTR_BGPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;JJZZB)V"

// -----[ cbgp_jni_new_IPPrefix ]------------------------------------
/**
 * This function creates a new instance of the Java IPPrefix object
 * from a CBGP prefix.
 */
jobject cbgp_jni_new_IPPrefix(JNIEnv * env, SPrefix sPrefix)
{
  jclass class_IPPrefix;
  jmethodID id_IPPrefix;
  jobject obj_IPPrefix;

  if ((class_IPPrefix= (*env)->FindClass(env, CLASS_IPPrefix)) == NULL)
    return NULL;

  if ((id_IPPrefix= (*env)->GetMethodID(env, class_IPPrefix, "<init>", CONSTR_IPPrefix)) == NULL)
    return NULL;

  if ((obj_IPPrefix= (*env)->NewObject(env, class_IPPrefix, id_IPPrefix,
				       (sPrefix.tNetwork >> 24) & 255,
				       (sPrefix.tNetwork >> 16) & 255,
				       (sPrefix.tNetwork >> 8) & 255,
				       (sPrefix.tNetwork & 255),
				       sPrefix.uMaskLen)) == NULL)
    return NULL;

  return obj_IPPrefix;
}

// -----[ cbgp_jni_new_IPAddress ]-----------------------------------
/**
 * This function creates a new instance of the Java IPAddress object
 * from a CBGP address.
 */
jobject cbgp_jni_new_IPAddress(JNIEnv * env, net_addr_t tAddr)
{
  jclass class_IPAddress;
  jmethodID id_IPAddress;
  jobject obj_IPAddress;

  if ((class_IPAddress= (*env)->FindClass(env, CLASS_IPAddress)) == NULL)
    return NULL;

  if ((id_IPAddress= (*env)->GetMethodID(env, class_IPAddress, "<init>",
				   CONSTR_IPAddress)) == NULL)
    return NULL;

  if ((obj_IPAddress= (*env)->NewObject(env, class_IPAddress, id_IPAddress,
					(tAddr >> 24) & 255,
					(tAddr >> 16) & 255,
					(tAddr >> 8) & 255,
					(tAddr & 255))) == NULL)
    return NULL;

  return obj_IPAddress;
}

// -----[ cbgp_jni_new_Link ]----------------------------------------
/**
 * This function creates a new instance of the Link object from a CBGP
 * link.
 */
jobject cbgp_jni_new_Link(JNIEnv * env, SNetLink * pLink)
{
  jclass class_Link;
  jmethodID id_Link;
  jobject obj_Link;

  /* Convert link attributes to Java objects */
  jobject obj_IPAddress= cbgp_jni_new_IPAddress(env, pLink->tAddr);

  /* Check that the conversion was successful */
  if (obj_IPAddress == NULL)
    return NULL;

  /* Create new Link object */
  if ((class_Link= (*env)->FindClass(env, CLASS_Link)) == NULL)
    return NULL;

  if ((id_Link= (*env)->GetMethodID(env, class_Link, "<init>",
				   CONSTR_Link)) == NULL)
    return NULL;

  if ((obj_Link= (*env)->NewObject(env, class_Link, id_Link,
				   obj_IPAddress,
				   (jlong) pLink->tDelay,
				   (jlong) pLink->uIGPweight,
				   (link_get_state(pLink, NET_LINK_FLAG_UP)?JNI_TRUE:JNI_FALSE))) == NULL)
    return NULL;

  return obj_Link;
}

// -----[ cbgp_jni_new_IPRoute ]-------------------------------------
/**
 * This function creates a new instance of the IPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_IPRoute(JNIEnv * env, SPrefix sPrefix, SNetRouteInfo * pRoute)
{
  jclass class_IPRoute;
  jmethodID id_IPRoute;
  jobject obj_IPRoute;

  /* Convert route attributes to Java objects */
  jobject obj_IPPrefix= cbgp_jni_new_IPPrefix(env, sPrefix);
  jobject obj_IPAddress= cbgp_jni_new_IPAddress(env, pRoute->pNextHopIf->tAddr);

  /* Check that the conversion was successful */
  if ((obj_IPPrefix == NULL) || (obj_IPAddress == NULL))
    return NULL;

  /* Create new IPRoute object */
  if ((class_IPRoute= (*env)->FindClass(env, CLASS_IPRoute)) == NULL)
    return NULL;

  if ((id_IPRoute= (*env)->GetMethodID(env, class_IPRoute, "<init>",
				   CONSTR_IPRoute)) == NULL)
    return NULL;

  if ((obj_IPRoute= (*env)->NewObject(env, class_IPRoute, id_IPRoute,
				      obj_IPPrefix, obj_IPAddress,
				      pRoute->tType)) == NULL)
    return NULL;

  return obj_IPRoute;
}

// -----[ cbgp_jni_new_BGPPeer ]-------------------------------------
/**
 * This function creates a new instance of the BGPPeer object from a
 * CBGP peer.
 */
jobject cbgp_jni_new_BGPPeer(JNIEnv * env, SPeer * pPeer)
{
  jclass class_BGPPeer;
  jmethodID id_BGPPeer;
  jobject obj_BGPPeer;

  /* Convert peer attributes to Java objects */
  jobject obj_IPAddress= cbgp_jni_new_IPAddress(env, pPeer->tAddr);

  /* Check that the conversion was successful */
  if (obj_IPAddress == NULL)
    return NULL;

  /* Create new BGPPeer object */
  if ((class_BGPPeer= (*env)->FindClass(env, CLASS_BGPPeer)) == NULL)
    return NULL;

  if ((id_BGPPeer= (*env)->GetMethodID(env, class_BGPPeer, "<init>",
				   CONSTR_BGPPeer)) == NULL)
    return NULL;

  if ((obj_BGPPeer= (*env)->NewObject(env, class_BGPPeer, id_BGPPeer,
				      obj_IPAddress,
				      (jint) pPeer->uRemoteAS,
				      (jbyte) pPeer->uSessionState)) == NULL)
    return NULL;

  return obj_BGPPeer;
}

// -----[ cbgp_jni_new_BGPRoute ]------------------------------------
/**
 * This function creates a new instance of the BGPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_BGPRoute(JNIEnv * env, SRoute * pRoute)
{
  jclass class_BGPRoute;
  jmethodID id_BGPRoute;
  jobject obj_BGPRoute;

  /* Convert route attributes to Java objects */
  jobject obj_IPPrefix= cbgp_jni_new_IPPrefix(env, pRoute->sPrefix);
  jobject obj_IPAddress= cbgp_jni_new_IPAddress(env, route_nexthop_get(pRoute));

  /* Check that the conversion was successful */
  if ((obj_IPPrefix == NULL) || (obj_IPAddress == NULL))
    return NULL;

  /* Create new BGPRoute object */
  if ((class_BGPRoute= (*env)->FindClass(env, CLASS_BGPRoute)) == NULL)
    return NULL;

  if ((id_BGPRoute= (*env)->GetMethodID(env, class_BGPRoute, "<init>",
				   CONSTR_BGPRoute)) == NULL)
    return NULL;

  if ((obj_BGPRoute= (*env)->NewObject(env, class_BGPRoute, id_BGPRoute,
				       obj_IPPrefix, obj_IPAddress,
				       (jlong) route_localpref_get(pRoute),
				       (jlong) route_med_get(pRoute),
				       (route_flag_get(pRoute, ROUTE_FLAG_BEST))?JNI_TRUE:JNI_FALSE,
				       (route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))?JNI_TRUE:JNI_FALSE,
				       route_origin_get(pRoute))) == NULL)
    return NULL;

  return obj_BGPRoute;
}

// -----[ cbgp_jni_new_Vector ]--------------------------------------
jobject cbgp_jni_new_Vector(JNIEnv * env)
{
  jclass jcVector;
  jmethodID jmVector;
  jobject joVector;

  if ((jcVector= (*env)->FindClass(env, "java/util/Vector")) == NULL)
    return NULL;
  if ((jmVector= (*env)->GetMethodID(env, jcVector, "<init>", "()V")) == NULL)
    return NULL;
  if ((joVector= (*env)->NewObject(env, jcVector, jmVector)) == NULL)
    return NULL;

  return joVector;
}

// -----[ cbgp_jni_Vector_add_rt_route ]-----------------------------
int cbgp_jni_Vector_add(JNIEnv * env, jobject joVector, jobject joObject)
{
  jclass jcVector;
  jmethodID jmVector;

  if ((jcVector= (*env)->GetObjectClass(env, joVector)) == NULL)
    return -1;
  if ((jmVector= (*env)->GetMethodID(env, jcVector, "addElement", "(Ljava/lang/Object;)V")) == NULL)
    return -1;

  (*env)->CallVoidMethod(env, joVector, jmVector, joObject);

  return 0;
}

// -----[ ip_jstring_to_address ]------------------------------------
/**
 * This function returns a CBGP address from a Java string.
 *
 * Returns 0 on success and -1 on error.
 */
int ip_jstring_to_address(JNIEnv * env, jstring jsAddr, net_addr_t * ptAddr)
{
  const jbyte * cNetAddr;
  char * pcEndPtr;
  int iResult= 0;

  if (jsAddr == NULL)
    return -1;

  cNetAddr = (*env)->GetStringUTFChars(env, jsAddr, NULL);
  if ((ip_string_to_address((char *)cNetAddr, &pcEndPtr, ptAddr) != 0) ||
      (*pcEndPtr != 0))
    iResult= -1;
  (*env)->ReleaseStringUTFChars(env, jsAddr, cNetAddr);

  return iResult;
}

// -----[ ip_jstring_to_prefix ]-------------------------------------
/**
 * This function returns a CBGP prefix from a Java string.
 */
int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix, SPrefix * psPrefix)
{
  const jbyte * cPrefix;
  char * pcEndPtr;
  int iResult= 0;

  if (jsPrefix == NULL)
    return -1;

  cPrefix = (*env)->GetStringUTFChars(env, jsPrefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, psPrefix) ||
      (*pcEndPtr != 0)) 
    iResult= -1;
  (*env)->ReleaseStringUTFChars(env, jsPrefix, cPrefix);

  return iResult;
}

// -----[ cbgp_jni_net_node_from_string ]----------------------------
SNetNode * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr)
{
  SNetwork * pNetwork = network_get();
  net_addr_t tNetAddr;

  if (ip_jstring_to_address(env, jsAddr, &tNetAddr) != 0)
    return NULL;
  return network_find_node(pNetwork, tNetAddr);
}

// -----[ cbgp_jni_bgp_router_from_string ]--------------------------
/**
 * This function returns a CBGP router from a string that contains its
 * IP address.
 */
SBGPRouter * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr)
{
  SNetNode * pNode;
  SNetProtocol * pProtocol;

  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return NULL;
  if ((pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP)) == NULL)
    return NULL;
  return pProtocol->pHandler;
}
