// ==================================================================
// @(#)jni_util.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// @lastdate 10/01/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni/exceptions.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

#include <bgp/as.h>
#include <bgp/comm.h>
#include <bgp/domain.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/error.h>
#include <net/igp_domain.h>
#include <net/link.h>
#include <net/network.h>
#include <net/node.h>
#include <jni.h>

#define CLASS_AbstractConsoleEventListener \
  "be/ac/ucl/ingi/cbgp/AbstractConsoleEventListener"
#define METHOD_AbstractConsoleEventListener_eventFired \
  "(Ljava/lang/String;)V"

#define CLASS_ASPath "be/ac/ucl/ingi/cbgp/bgp/ASPath"
#define CONSTR_ASPath "()V"
#define METHOD_ASPath_append "(Lbe/ac/ucl/ingi/cbgp/bgp/ASPathSegment;)V"

#define CLASS_ASPathSegment "be/ac/ucl/ingi/cbgp/bgp/ASPathSegment"
#define CONSTR_ASPathSegment "(I)V"

#define CLASS_ConsoleEvent "be/ac/ucl/ingi/cbgp/ConsoleEvent"
#define CONSTR_ConsoleEvent "(Ljava/lang/String;)V"

#define CLASS_IPPrefix "be/ac/ucl/ingi/cbgp/IPPrefix"
#define CONSTR_IPPrefix "(BBBBB)V"

#define CLASS_IPAddress "be/ac/ucl/ingi/cbgp/IPAddress"
#define CONSTR_IPAddress "(BBBB)V"

#define CLASS_IPRoute "be/ac/ucl/ingi/cbgp/IPRoute"
#define CONSTR_IPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;B)V"

#define CLASS_BGPRoute "be/ac/ucl/ingi/cbgp/bgp/Route"
#define CONSTR_BGPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;JJZZB" \
                        "Lbe/ac/ucl/ingi/cbgp/bgp/ASPath;Z" \
                        "Lbe/ac/ucl/ingi/cbgp/bgp/Communities;)V"

#define CLASS_MessageUpdate "be/ac/ucl/ingi/cbgp/bgp/MessageUpdate"
#define CONSTR_MessageUpdate \
  "(Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/bgp/Route;)V"

#define CLASS_MessageWithdraw "be/ac/ucl/ingi/cbgp/bgp/MessageWithdraw"
#define CONSTR_MessageWithdraw \
  "(Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/IPPrefix;)V"

#define CLASS_Communities "be/ac/ucl/ingi/cbgp/bgp/Communities"
#define CONSTR_Communities "()V"
#define METHOD_Communities_append "(I)V"

// -----[ cbgp_jni_new_Communities ]---------------------------------
/**
 * Build a new Communities object from a C-BGP Communities.
 */
jobject cbgp_jni_new_Communities(JNIEnv * env, SCommunities * pComm)
{
  jobject joCommunities;
  int iIndex;

  if (pComm == NULL)
    return NULL;

  /* Build new Communities object */
  if ((joCommunities= cbgp_jni_new(env, CLASS_Communities,
				   CONSTR_Communities)) == NULL)
    return NULL;

  /* Append all communities */
  for (iIndex= 0; iIndex < pComm->uNum; iIndex++) {
    if (cbgp_jni_Communities_append(env, joCommunities,
				    (comm_t) pComm->asComms[iIndex]) != 0)
      return NULL;
  }

  return joCommunities;
}

// -----[ cbgp_jni_Communities_append ]------------------------------
/**
 * Append a community to a Communities object.
 */
int cbgp_jni_Communities_append(JNIEnv * env, jobject joCommunities,
				comm_t tCommunity)
{
  return cbgp_jni_call_void(env, joCommunities,
			    "append", METHOD_Communities_append,
			    (jint) tCommunity);
}

// -----[ cbgp_jni_new_ASPath ]--------------------------------------
/**
 * Build a new ASPath object from a C-BGP AS-Path.
 */
jobject cbgp_jni_new_ASPath(JNIEnv * env, SBGPPath * pPath)
{
  jobject joASPath;
  int iIndex;
  SPathSegment * pSegment;

  /* Build new ASPath object */
  if ((joASPath= cbgp_jni_new(env, CLASS_ASPath, CONSTR_ASPath)) == NULL)
    return NULL;

  /* Append all AS-Path segments */
  for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++) {
    pSegment= (SPathSegment *) pPath->data[iIndex];
    if (cbgp_jni_ASPath_append(env, joASPath, pSegment) != 0)
      return NULL;
  }

  return joASPath;
}

// -----[ cbgp_jni_ASPath_append ]-----------------------------------
/**
 * Append an AS-Path segment to an ASPath object.
 */
int cbgp_jni_ASPath_append(JNIEnv * env, jobject joASPath,
			   SPathSegment * pSegment)
{
  jobject joASPathSeg;

  /* Convert the AS-Path segment to an ASPathSegment object. */
  if ((joASPathSeg= cbgp_jni_new_ASPathSegment(env, pSegment)) == NULL)
    return -1;

  return cbgp_jni_call_void(env, joASPath,
			    "append", METHOD_ASPath_append,
			    joASPathSeg);
}

// -----[ cbgp_jni_new_ASPathSegment ]-------------------------------
jobject cbgp_jni_new_ASPathSegment(JNIEnv * env, SPathSegment * pSegment)
{
  jobject joASPathSeg;
  int iIndex;

  if ((joASPathSeg= cbgp_jni_new(env, CLASS_ASPathSegment,
				 CONSTR_ASPathSegment,
				 (jint) pSegment->uType)) == NULL)
    return NULL;

  /* Append all AS-Path segment elements */
  for (iIndex= 0; iIndex < pSegment->uLength; iIndex++) {
    if (cbgp_jni_ASPathSegment_append(env, joASPathSeg,
				  pSegment->auValue[iIndex]) != 0)
      return NULL;
  }

  return joASPathSeg;
}

// -----[ cbgp_jni_ASPathSegment_append ]----------------------------
/**
 *
 */
int cbgp_jni_ASPathSegment_append(JNIEnv * env, jobject joASPathSeg,
				  uint16_t uAS)
{
  jclass jcASPathSeg;
  jmethodID jmASPathSeg;

  if ((jcASPathSeg= (*env)->GetObjectClass(env, joASPathSeg)) == NULL)
    return -1;
  if ((jmASPathSeg= (*env)->GetMethodID(env, jcASPathSeg, "append",
					"(I)V")) == NULL)
    return -1;

  (*env)->CallVoidMethod(env, joASPathSeg, jmASPathSeg, (jint) uAS);

  return 0;
}

// -----[ cbgp_jni_new_ConsoleEvent ]--------------------------------
/**
 * Build a new ConsoleEvent object.
 */
jobject cbgp_jni_new_ConsoleEvent(JNIEnv * jEnv, char * pcMessage)
{
  jobject joConsoleEvent= NULL;
  jstring jsMessage;

  /* Build a Java String */
  jsMessage= cbgp_jni_new_String(jEnv, pcMessage);

  // Build new ConsoleEvent object.
  if ((joConsoleEvent= cbgp_jni_new(jEnv, CLASS_ConsoleEvent,
				    CONSTR_ConsoleEvent,
				    jsMessage)) == NULL)
    return NULL;

  return joConsoleEvent;
}

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
				       (jbyte) ((sPrefix.tNetwork >> 24) & 255),
				       (jbyte) ((sPrefix.tNetwork >> 16) & 255),
				       (jbyte) ((sPrefix.tNetwork >> 8) & 255),
				       (jbyte) (sPrefix.tNetwork & 255),
				       (jbyte) sPrefix.uMaskLen)) == NULL)
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
					(jbyte) ((tAddr >> 24) & 255),
					(jbyte) ((tAddr >> 16) & 255),
					(jbyte) ((tAddr >> 8) & 255),
					(jbyte) (tAddr & 255))) == NULL)
    return NULL;

  return obj_IPAddress;
}

// -----[ cbgp_jni_new_IPRoute ]-------------------------------------
/**
 * This function creates a new instance of the IPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_IPRoute(JNIEnv * jEnv, SPrefix sPrefix,
			     SNetRouteInfo * pRoute)
{
  jclass class_IPRoute;
  jmethodID id_IPRoute;
  jobject joRoute;
  jobject joPrefix;
  jobject joGateway;
  jobject joIface;
  SPrefix sIface;

  /* Convert route attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, sPrefix);
  if (joPrefix == NULL)
    return NULL;

  sIface= net_link_get_id(pRoute->sNextHop.pIface);
  joIface=
    cbgp_jni_new_IPAddress(jEnv, sIface.tNetwork);
  if (joIface == NULL)
    return NULL;

  joGateway= cbgp_jni_new_IPAddress(jEnv, pRoute->sNextHop.tGateway);
  if (joGateway == NULL)
    return NULL;

  /* Create new IPRoute object */
  if ((class_IPRoute= (*jEnv)->FindClass(jEnv, CLASS_IPRoute)) == NULL)
    return NULL;

  if ((id_IPRoute= (*jEnv)->GetMethodID(jEnv, class_IPRoute, "<init>",
				   CONSTR_IPRoute)) == NULL)
    return NULL;

  if ((joRoute= (*jEnv)->NewObject(jEnv, class_IPRoute, id_IPRoute,
				   joPrefix, joIface, joGateway,
				   pRoute->tType)) == NULL)
    return NULL;

  return joRoute;
}

// -----[ cbgp_jni_new_LinkMetrics ]---------------------------------
jobject cbgp_jni_new_LinkMetrics(JNIEnv * jEnv,
				 net_link_delay_t tDelay,
				 net_link_delay_t tWeight)
{
  return NULL;
}

// -----[ cbgp_jni_new_BGPRoute ]------------------------------------
/**
 * This function creates a new instance of the BGPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_BGPRoute(JNIEnv * jEnv, SRoute * pRoute,
			      jobject joHashtable)
{
  jobject joPrefix;
  jobject joNextHop;
  jobject joASPath= NULL;
  jobject joCommunities= NULL;
  jobject joKey;

  /* Convert route attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, pRoute->sPrefix);
  joNextHop= cbgp_jni_new_IPAddress(jEnv, route_get_nexthop(pRoute));

  /* Check that the conversion was successful */
  if ((joPrefix == NULL) || (joNextHop == NULL))
    return NULL;

  /* Convert AS-Path or retrieve from hashtable */
  if (pRoute->pAttr->pASPathRef != NULL) {
    if (joHashtable != NULL) {
#ifdef __LP64__
      joKey= cbgp_jni_new_Long(jEnv, (jlong) pRoute->pAttr->pASPathRef);
#else
      joKey= cbgp_jni_new_Integer(jEnv, (jint) pRoute->pAttr->pASPathRef);
#endif
      joASPath= cbgp_jni_Hashtable_get(jEnv, joHashtable, joKey);
      if (joASPath == NULL) {
	joASPath= cbgp_jni_new_ASPath(jEnv, pRoute->pAttr->pASPathRef);
	if (joASPath == NULL)
	  return NULL;
	cbgp_jni_Hashtable_put(jEnv, joHashtable, joKey, joASPath);
      }
    } else {
      joASPath= cbgp_jni_new_ASPath(jEnv, pRoute->pAttr->pASPathRef);
      if (joASPath == NULL)
	return NULL;
    }
  }

  /* Convert Communities or retrieve from hashtable */
  if (pRoute->pAttr->pCommunities != NULL) {
    if (joHashtable != NULL) {
#ifdef __LP64__
      joKey= cbgp_jni_new_Long(jEnv, (jlong) pRoute->pAttr->pCommunities);
#else
      joKey= cbgp_jni_new_Integer(jEnv, (jint) pRoute->pAttr->pCommunities);
#endif
      joCommunities= cbgp_jni_Hashtable_get(jEnv, joHashtable, joKey);
      if (joCommunities == NULL) {
	joCommunities=
	  cbgp_jni_new_Communities(jEnv, pRoute->pAttr->pCommunities);
	if (joCommunities == NULL)
	  return NULL;
	cbgp_jni_Hashtable_put(jEnv, joHashtable, joKey, joCommunities);
      }
    } else {
      joCommunities=
	cbgp_jni_new_Communities(jEnv, pRoute->pAttr->pCommunities);
      if (joCommunities == NULL)
	return NULL;
    }
  }

  /* Create new BGPRoute object */
  return cbgp_jni_new(jEnv, CLASS_BGPRoute, CONSTR_BGPRoute,
		      joPrefix,
		      joNextHop,
		      NULL,
		      (jlong) route_localpref_get(pRoute),
		      (jlong) route_med_get(pRoute),
		      (route_flag_get(pRoute, ROUTE_FLAG_BEST))?JNI_TRUE:JNI_FALSE,
		      (route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))?JNI_TRUE:JNI_FALSE,
		      route_get_origin(pRoute),
		      joASPath,
		      (route_flag_get(pRoute, ROUTE_FLAG_INTERNAL))?JNI_TRUE:JNI_FALSE,
		      joCommunities);
}

// -----[ cbgp_jni_new_BGPMessage ]----------------------------------
/**
 * This function creates a new instance of the bgp.Message object
 * from a CBGP message.
 */
jobject cbgp_jni_new_BGPMessage(JNIEnv * jEnv, SNetMessage * pMessage)
{
  jobject joFrom= cbgp_jni_new_IPAddress(jEnv, pMessage->tSrcAddr);
  jobject joTo= cbgp_jni_new_IPAddress(jEnv, pMessage->tDstAddr);
  jobject joMessage= NULL;
  jobject joRoute= NULL;
  jobject joPrefix= NULL;
  SBGPMsg * pMsg= (SBGPMsg *) pMessage->pPayLoad;
  SBGPMsgUpdate * pMsgUpdate;
  SBGPMsgWithdraw * pMsgWithdraw;
  
  /* Check that the conversion was successful */
  if ((joFrom == NULL) || (joTo == NULL))
    return NULL;

  switch (pMsg->uType) {
  case BGP_MSG_TYPE_UPDATE:
    pMsgUpdate= (SBGPMsgUpdate *) pMsg;
    if ((joRoute= cbgp_jni_new_BGPRoute(jEnv, pMsgUpdate->pRoute, NULL)) == NULL)
      return NULL;
    joMessage= cbgp_jni_new(jEnv, CLASS_MessageUpdate,
			    CONSTR_MessageUpdate,
			    joFrom, joTo, joRoute);
    break;

  case BGP_MSG_TYPE_WITHDRAW:
    pMsgWithdraw= (SBGPMsgWithdraw *) pMsg;
    if ((joPrefix= cbgp_jni_new_IPPrefix(jEnv, pMsgWithdraw->sPrefix)) == NULL)
      return NULL;
    joMessage= cbgp_jni_new(jEnv, CLASS_MessageWithdraw,
			    CONSTR_MessageWithdraw,
			    joFrom, joTo, joPrefix);
    break;

  default:
    return NULL;
  }

  /* Create new bgp.Message object */
  return joMessage;
}

// -----[ ip_jstring_to_address ]------------------------------------
/**
 * This function returns a CBGP address from a Java string.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
int ip_jstring_to_address(JNIEnv * env, jstring jsAddr, net_addr_t * ptAddr)
{
  const char * cNetAddr;
  char * pcEndPtr;
  int iResult= 0;

  if (jsAddr == NULL)
    return -1;

  cNetAddr = (*env)->GetStringUTFChars(env, jsAddr, NULL);

  if ((ip_string_to_address((char *)cNetAddr, &pcEndPtr, ptAddr) != 0) ||
      (*pcEndPtr != 0))
    iResult= -1;
  (*env)->ReleaseStringUTFChars(env, jsAddr, cNetAddr);

  /* If the conversion could not be performed, throw an Exception */
  if (iResult != 0)
    throw_CBGPException(env, "Invalid IP address");

  return iResult;
}

// -----[ ip_jstring_to_prefix ]-------------------------------------
/**
 * This function returns a CBGP prefix from a Java string.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix, SPrefix * psPrefix)
{
  const char * cPrefix;
  char * pcEndPtr;
  int iResult= 0;

  if (jsPrefix == NULL)
    return -1;

  cPrefix = (*env)->GetStringUTFChars(env, jsPrefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, psPrefix) ||
      (*pcEndPtr != 0)) 
    iResult= -1;
  (*env)->ReleaseStringUTFChars(env, jsPrefix, cPrefix);

  /* If the conversion could not be performed, throw an Exception */
  if (iResult != 0)
    throw_CBGPException(env, "Invalid IP prefix");

  return iResult;
}

// -----[ ip_jstring_to_dest ]---------------------------------------
/**
 * Convert the destination to a SNetDest structure.
 */
int ip_jstring_to_dest(JNIEnv * jEnv, jstring jsDest, SNetDest * pDest)
{
  const char * cDest;
  int iResult= 0;

  if (jsDest == NULL)
    return -1;

  cDest= (*jEnv)->GetStringUTFChars(jEnv, jsDest, NULL);
  if (ip_string_to_dest((char *)cDest, pDest) != 0) 
    iResult= -1;
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsDest, cDest);

  /* If the conversion could not be performed, throw an Exception */
  if (iResult != 0)
    throw_InvalidDestination(jEnv, jsDest);

  return iResult;
}


// -----[ cbgp_jni_net_node_from_string ]----------------------------
/**
 * Get the C-BGP node identified by the String that contains its IP
 * address.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
SNetNode * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr)
{
  SNetNode * pNode;
  net_addr_t tNetAddr;

  if (ip_jstring_to_address(env, jsAddr, &tNetAddr) != 0)
    return NULL;

  if ((pNode= network_find_node(tNetAddr)) == NULL) {
    throw_CBGPException(env, "could not find node");
    return NULL;
  }
    
  return pNode;
}

// -----[ cbgp_jni_net_link_from_string ]----------------------------
/**
 * Get the link identified by the Strings that contain the source and
 * destination nodes' IP addresses.
 */
SNetLink * cbgp_jni_net_link_from_string(JNIEnv * env, jstring jsSrcAddr,
					 jstring jsDstAddr)
{
  SNetNode * pNode1, * pNode2;
  SNetLink * pLink;

  if ((pNode1= cbgp_jni_net_node_from_string(env, jsSrcAddr)) == NULL)
    return NULL;
  if ((pNode2= cbgp_jni_net_node_from_string(env, jsDstAddr)) == NULL)
    return NULL;
  
  if ((pLink= node_find_link_ptp(pNode1, pNode2->tAddr)) == NULL) {
    throw_CBGPException(env, "could not find link");
    return NULL;
  }

  return pLink;
}

// -----[ cbgp_jni_bgp_router_from_string ]--------------------------
/**
 * Get the C-BGP router identified by the String that contains its
 * IP address.
 *
 * @return NULL is the router was not found
 *
 * @throw CBGPException
 */
SBGPRouter * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr)
{
  SNetNode * pNode;
  SNetProtocol * pProtocol;

  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return NULL;

  if (((pProtocol= protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP)) == NULL) || (pProtocol->pHandler == NULL)) {
    throw_CBGPException(env, "node does not support BGP");
    return NULL;
  }

  return pProtocol->pHandler;
}

// -----[ cbgp_jni_bgp_peer_from_string ]----------------------------
/**
 * Get the BGP peer identified by the Strings that contain the router
 * and peer's IP addresses.
 *
 * @return NULL if the peer was not found
 *
 * @throw CBGPException
 */
SBGPPeer * cbgp_jni_bgp_peer_from_string(JNIEnv * env, jstring jsRouterAddr,
					 jstring jsPeerAddr)
{
  SBGPRouter * pRouter;
  net_addr_t tPeerAddr;
  SBGPPeer * pPeer;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return NULL;
  if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
    return NULL;

  pPeer= bgp_router_find_peer(pRouter, tPeerAddr);
  if (pPeer == NULL) {
    throw_CBGPException(env, "peer not found");
    return NULL;
  }

  return pPeer;
}

// -----[ cbgp_jni_net_domain_from_int ]--------------------------
/**
 * Get the C-BGP domain identified by the String that contains its ID.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
SIGPDomain * cbgp_jni_net_domain_from_int(JNIEnv * env, jint iNumber)
{
  // Check that domain ID is in the range [0, 65535]
  if ((iNumber < 0) || (iNumber > 65535)) {
    throw_CBGPException(env, "value out of bound");
    return NULL;
  }

  // Check that the domain exists
  if (!exists_igp_domain((uint16_t) iNumber)) {
    throw_CBGPException(env, "could not find domain");
    return NULL;
  }

  return get_igp_domain((uint16_t) iNumber);
}

// -----[ cbgp_jni_bgp_domain_from_int ]--------------------------
/**
 * Get the C-BGP domain identified by the String that contains its ID.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
SBGPDomain * cbgp_jni_bgp_domain_from_int(JNIEnv * env, jint iNumber)
{
  // Check that domain ID is in the range [0, 65535]
  if ((iNumber < 0) || (iNumber > 65535)) {
    throw_CBGPException(env, "value out of bound");
    return NULL;
  }

  // Check that the domain exists
  if (!exists_bgp_domain((uint16_t) iNumber)) {
    throw_CBGPException(env, "could not find domain");
    return NULL;
  }

  return get_bgp_domain((uint16_t) iNumber);
}

// -----[ cbgp_jni_net_error_str ]-----------------------------------
jstring cbgp_jni_net_error_str(JNIEnv * jEnv, int iErrorCode)
{
  return cbgp_jni_new_String(jEnv, network_strerror(iErrorCode));
}


