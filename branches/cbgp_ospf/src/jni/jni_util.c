// ==================================================================
// @(#)jni_util.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 05/03/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni/jni_util.h>

#include <bgp/as_t.h>
#include <bgp/as.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/link.h>
#include <net/network.h>
#include <jni.h>

#define CLASS_CBGPException "be/ac/ucl/ingi/cbgp/CBGPException"

#define CLASS_ASPath "be/ac/ucl/ingi/cbgp/ASPath"
#define CONSTR_ASPath "()V"
#define METHOD_ASPath_append "(Lbe/ac/ucl/ingi/cbgp/ASPathSegment;)V"

#define CLASS_ASPathSegment "be/ac/ucl/ingi/cbgp/ASPathSegment"
#define CONSTR_ASPathSegment "(I)V"

#define CLASS_IPPrefix "be/ac/ucl/ingi/cbgp/IPPrefix"
#define CONSTR_IPPrefix "(BBBBB)V"

#define CLASS_IPAddress "be/ac/ucl/ingi/cbgp/IPAddress"
#define CONSTR_IPAddress "(BBBB)V"

#define CLASS_Link "be/ac/ucl/ingi/cbgp/Link"
#define CONSTR_Link "(Lbe/ac/ucl/ingi/cbgp/IPAddress;JJZ)V"

#define CLASS_IPRoute "be/ac/ucl/ingi/cbgp/IPRoute"
#define CONSTR_IPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;B)V"

#define CLASS_IPTrace "be/ac/ucl/ingi/cbgp/IPTrace"
#define CONSTR_IPTrace "(Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;III)V"
#define METHOD_IPTrace_append "(Lbe/ac/ucl/ingi/cbgp/IPAddress;)V"

#define CLASS_BGPPeer "be/ac/ucl/ingi/cbgp/BGPPeer"
#define CONSTR_BGPPeer "(Lbe/ac/ucl/ingi/cbgp/IPAddress;IBB)V"

#define CLASS_BGPRoute "be/ac/ucl/ingi/cbgp/BGPRoute"
#define CONSTR_BGPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;JJZZB" \
                        "Lbe/ac/ucl/ingi/cbgp/ASPath;)V"

// -----[ cbgp_jni_new ]---------------------------------------------
/**
 * Creates a new Java Object.
 */
jobject cbgp_jni_new(JNIEnv * env,
		     const char * pcClass,
		     const char * pcConstr,
		     ...)
{
  va_list ap;
  jclass jcObject;
  jmethodID jmObject;

  va_start(ap, pcConstr);

  /* Get the object's class */
  if ((jcObject= (*env)->FindClass(env, pcClass)) == NULL)
    return NULL;

  /* Get the constructor */
  if ((jmObject= (*env)->GetMethodID(env, jcObject,
				     "<init>", pcConstr)) == NULL)
    return NULL;

  /* Build new object... */
  return (*env)->NewObjectV(env, jcObject, jmObject, ap);
}

// -----[ cbgp_jni_call_void ]---------------------------------------
/**
 * Call a void method of the given object.
 */
int cbgp_jni_call_void(JNIEnv * env, jobject joObject,
		       const char * pcMethod,
		       const char * pcSignature, ...)
{
  va_list ap;
  jclass jcObject;
  jmethodID jmObject;

  va_start(ap, pcSignature);

  /* Get the object's class */
  if ((jcObject= (*env)->GetObjectClass(env, joObject)) == NULL)
    return -1;

  /* Get the method */
  if ((jmObject= (*env)->GetMethodID(env, jcObject, pcMethod,
				     pcSignature)) == NULL)
    return -1;

  /* Call the method */
  (*env)->CallVoidMethodV(env, joObject, jmObject, ap);

  return 0;
}

// -----[ cbgp_jni_call_boolean ]------------------------------------
/**
 * Call a void method of the given object.
 */
int cbgp_jni_call_boolean(JNIEnv * env, jobject joObject,
			  const char * pcMethod,
			  const char * pcSignature, ...)
{
  va_list ap;
  jclass jcObject;
  jmethodID jmObject;

  va_start(ap, pcSignature);

  /* Get the object's class */
  if ((jcObject= (*env)->GetObjectClass(env, joObject)) == NULL)
    return -1;

  /* Get the method */
  if ((jmObject= (*env)->GetMethodID(env, jcObject, pcMethod,
				     pcSignature)) == NULL)
    return -1;

  /* Call the method */
  (*env)->CallBooleanMethodV(env, joObject, jmObject, ap);

  return 0;
}

// -----[ cbgp_jni_throw_CBGPException ]-----------------------------
/**
 * Throw a CBGPException.
 */
void cbgp_jni_throw_CBGPException(JNIEnv * env, char * pcMsg)
{
  jclass jcException;

  jcException= (*env)->FindClass(env, CLASS_CBGPException);
  if (jcException != NULL)
      (*env)->ThrowNew(env, jcException, pcMsg);
}


// -----[ cbgp_jni_new_ASPath ]--------------------------------------
/**
 * Build a new ASPath object from a C-BGP AS-Path.
 */
jobject cbgp_jni_new_ASPath(JNIEnv * env, SPath * pPath)
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

// -----[ cbgp_jni_IPTrace_for_each ]--------------------------------
int cbgp_jni_IPTrace_for_each(void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  net_addr_t tAddress= *(net_addr_t *) pItem;
  jobject joAddress;

  if ((joAddress= cbgp_jni_new_IPAddress(pCtx->jEnv, tAddress)) == NULL)
    return -1;

  return cbgp_jni_call_void(pCtx->jEnv, pCtx->joVector,
			    "append",
			    METHOD_IPTrace_append,
			    joAddress);
}

// -----[ cbgp_jni_new_IPTrace ]-------------------------------------
/**
 *
 */
jobject cbgp_jni_new_IPTrace(JNIEnv * env, net_addr_t tSrc,
			     net_addr_t tDst, SNetPath * pPath,
			     int iStatus, net_link_delay_t tDelay,
			     net_link_delay_t tWeight)
{
  jobject joIPTrace;
  jobject joSrc, joDst;
  SRouteDumpCtx sCtx;

  /* Convert src/dst to Java objects */
  if ((joSrc= cbgp_jni_new_IPAddress(env, tSrc)) == NULL)
    return NULL;
  if ((joDst= cbgp_jni_new_IPAddress(env, tDst)) == NULL)
    return NULL;

  /* Create new IPTrace object */
  if ((joIPTrace= cbgp_jni_new(env, CLASS_IPTrace, CONSTR_IPTrace,
			       joSrc, joDst, (jint) iStatus,
			       (jint) tDelay, (jint) tWeight)) == NULL)
    return NULL;

  /* Add hops */
  sCtx.jEnv= env;
  sCtx.joVector= joIPTrace;
  if (net_path_for_each(pPath, cbgp_jni_IPTrace_for_each, &sCtx) != 0)
    return NULL;

  return joIPTrace;
}

// -----[ cbgp_jni_new_BGPPeer ]-------------------------------------
/**
 * This function creates a new instance of the BGPPeer object from a
 * CBGP peer.
 */
jobject cbgp_jni_new_BGPPeer(JNIEnv * env, SPeer * pPeer)
{
  jobject joIPAddress;

  /* Convert peer attributes to Java objects */
  if ((joIPAddress= cbgp_jni_new_IPAddress(env, pPeer->tAddr)) == NULL)
    return NULL;

  /* Create new BGPPeer object */
  return cbgp_jni_new(env, CLASS_BGPPeer, CONSTR_BGPPeer,
		      joIPAddress,
		      (jint) pPeer->uRemoteAS,
		      (jbyte) pPeer->uSessionState,
		      (jbyte) pPeer->uFlags);
}

// -----[ cbgp_jni_new_BGPRoute ]------------------------------------
/**
 * This function creates a new instance of the BGPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_BGPRoute(JNIEnv * env, SRoute * pRoute)
{
  jobject joASPath;

  /* Convert route attributes to Java objects */
  jobject joIPPrefix= cbgp_jni_new_IPPrefix(env, pRoute->sPrefix);
  jobject joIPAddress= cbgp_jni_new_IPAddress(env, route_nexthop_get(pRoute));
  
  if (pRoute->pASPath != NULL) {
    if ((joASPath= cbgp_jni_new_ASPath(env, pRoute->pASPath)) == NULL)
      return NULL;
  } else {
    joASPath= NULL;
  }

  /* Check that the conversion was successful */
  if ((joIPPrefix == NULL) || (joIPAddress == NULL))
    return NULL;

  /* Create new BGPRoute object */
  return cbgp_jni_new(env, CLASS_BGPRoute, CONSTR_BGPRoute,
		      joIPPrefix, joIPAddress,
		      (jlong) route_localpref_get(pRoute),
		      (jlong) route_med_get(pRoute),
		      (route_flag_get(pRoute, ROUTE_FLAG_BEST))?JNI_TRUE:JNI_FALSE,
		      (route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))?JNI_TRUE:JNI_FALSE,
		      route_origin_get(pRoute),
		      joASPath);
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

// -----[ cbgp_jni_Vector_add ]--------------------------------------
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

// -----[ cbgp_jni_new_ArrayList ]-----------------------------------
jobject cbgp_jni_new_ArrayList(JNIEnv * env)
{
  return cbgp_jni_new(env, "java/util/ArrayList", "()V");
}

// -----[ cbgp_jni_ArrayList_add ]-----------------------------------
int cbgp_jni_ArrayList_add(JNIEnv * env, jobject joArrayList,
			   jobject joItem)
{
  return cbgp_jni_call_boolean(env, joArrayList,
			       "add", "(Ljava/lang/Object;)Z",
			       joItem);
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

  /* If the conversion could not be performed, throw an Exception */
  if (iResult != 0)
    cbgp_jni_throw_CBGPException(env, "Invalid IP address");

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

  /* If the conversion could not be performed, throw an Exception */
  if (iResult != 0)
    cbgp_jni_throw_CBGPException(env, "Invalid IP prefix");

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
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  net_addr_t tNetAddr;

  if (ip_jstring_to_address(env, jsAddr, &tNetAddr) != 0)
    return NULL;

  if ((pNode= network_find_node(pNetwork, tNetAddr)) == NULL) {
    cbgp_jni_throw_CBGPException(env, "could not find node");
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
  
  if ((pLink= node_find_link(pNode1, pNode2->tAddr)) == NULL) {
    cbgp_jni_throw_CBGPException(env, "could not find link");
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
    cbgp_jni_throw_CBGPException(env, "node does not support BGP");
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
    cbgp_jni_throw_CBGPException(env, "peer not found");
    return NULL;
  }

  return pPeer;
}
