// ==================================================================
// @(#)net_Node.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 19/04/2006
// @lastdate 23/11/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/net_IPTrace.h>
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/bgp_Router.h>

#include <net/error.h>
#include <net/node.h>
#include <net/record-route.h>

#define CLASS_Node "be/ac/ucl/ingi/cbgp/net/Node"
#define CONSTR_Node "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                    "Lbe/ac/ucl/ingi/cbgp/IPAddress;)V"

// -----[ cbgp_jni_new_net_Node ]------------------------------------
/**
 * This function creates a new instance of the Node object from a CBGP
 * node.
 */
jobject cbgp_jni_new_net_Node(JNIEnv * jEnv, jobject joCBGP,
			      SNetNode * pNode)
{
  jobject joNode;
  jobject joAddress;

  /* Java proxy object already existing ? */
  joNode= jni_proxy_get(jEnv, pNode);
  if (joNode != NULL)
    return joNode;

  /* Convert node attributes to Java objects */
  joAddress= cbgp_jni_new_IPAddress(jEnv, pNode->tAddr);

  /* Check that the conversion was successful */
  if (joAddress == NULL)
    return NULL;

  /* Create new Node object */
  if ((joNode= cbgp_jni_new(jEnv, CLASS_Node, CONSTR_Node,
			    joCBGP,
			    joAddress)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joNode, pNode);

  return joNode;
}

// -----[ getName ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getName
  (JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;
  jstring jsName= NULL;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the name */
  if (pNode->pcName != NULL)
    jsName= cbgp_jni_new_String(jEnv, pNode->pcName);

  jni_unlock(jEnv);
  return jsName;
}

// -----[ setName ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    setName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setName
  (JNIEnv * jEnv, jobject joNode, jstring jsName)
{
  SNetNode * pNode;
  const char * pcName;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  /* Set the name */
  if (jsName != NULL) {
    pcName= (*jEnv)->GetStringUTFChars(jEnv, jsName, NULL);
    node_set_name(pNode, pcName);
    (*jEnv)->ReleaseStringUTFChars(jEnv, jsName, pcName);
  } else
    node_set_name(pNode, NULL);

  jni_unlock(jEnv);
}

// -----[ recordRoute ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    recordRoute
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPTrace;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_recordRoute
  (JNIEnv * jEnv, jobject joNode, jstring jsDestination)
{
  SNetNode * pNode;
  jobject joIPTrace;
  SNetDest sDest;
  SNetRecordRouteInfo * pRRInfo;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the destination */
  if (ip_jstring_to_dest(jEnv, jsDestination, &sDest) != 0)
    return_jni_unlock(jEnv, NULL);

  /* "Any" destination not allowed here */
  if (sDest.tType == NET_DEST_ANY) {
    cbgp_jni_throw_CBGPException(jEnv, "invalid destination (*)");
    return_jni_unlock(jEnv, NULL);
  }

  /* Trace the IP-level route */
  pRRInfo= node_record_route(pNode, sDest, 0,
			     NET_RECORD_ROUTE_OPTION_DELAY |
			     NET_RECORD_ROUTE_OPTION_WEIGHT,
			     0);

  /* Convert to an IPTrace object */
  joIPTrace= cbgp_jni_new_IPTrace(jEnv, pNode->tAddr, sDest.uDest.tAddr, pRRInfo);

  net_record_route_info_destroy(&pRRInfo);

  return_jni_unlock(jEnv, joIPTrace);
}

// -----[ traceRoute ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    traceRoute
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPTrace;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_traceRoute
  (JNIEnv * jEnv, jobject joNode, jstring jsDestination)
{
  SNetNode * pNode;
  jobject joIPTrace;
  SNetDest sDest;
  SNetRecordRouteInfo sRRInfo;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination */
  if (ip_jstring_to_address(jEnv, jsDestination, &sDest.uDest.tAddr) != 0)
    return_jni_unlock(jEnv, NULL);
  sDest.tType= NET_DEST_ADDRESS;

  /* Trace the IP-level route */
  sRRInfo.pPath= net_path_create();
  sRRInfo.iResult=
    icmp_trace_route(NULL, pNode, NET_ADDR_ANY, sDest.uDest.tAddr,
		     0, sRRInfo.pPath);

  /* Convert to an IPTrace object */
  joIPTrace= cbgp_jni_new_IPTrace(jEnv, pNode->tAddr, sDest.uDest.tAddr, &sRRInfo);

  net_path_destroy(&sRRInfo.pPath);

  return_jni_unlock(jEnv, joIPTrace);
}

// -----[ _getAddresses ]--------------------------------------------
/**
 *
 */
static int _getAddresses(void * pItem, void * pContext)
{
  net_addr_t * pAddr= (net_addr_t *) pItem;
  SJNIContext * pCtx= (SJNIContext *) pContext;
  jobject joAddress;

  if ((joAddress= cbgp_jni_new_IPAddress(pCtx->jEnv, *pAddr)) == NULL)
    return -1;
  
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joAddress);
}

// -----[ getAddresses ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getAddresses
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getAddresses
  (JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;
  jobject joVector= NULL;
  SJNIContext sCtx;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;

  if (node_addresses_for_each(pNode, _getAddresses, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ addLTLLink ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addLTLLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Node;Z)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addLTLLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jboolean jbBidir)
{
  SNetNode * pNode, *pNodeDst;
  SNetLink * pLink;
  jobject joLink;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the destination */
  pNodeDst= (SNetNode*) jni_proxy_lookup(jEnv, joDst);
  if (pNodeDst == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Add links
   * params: src, dst, delay, capacity, depth, bidir */
  if (node_add_link_ptp(pNode, pNodeDst, 0, 0, 1,
			(jbBidir == JNI_TRUE)?1:0) < 0) {
    cbgp_jni_throw_CBGPException(jEnv, "link already exists");
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  pLink= node_find_link_ptp(pNode, pNodeDst->tAddr);
  joLink= cbgp_jni_new_net_Link(jEnv, jni_proxy_get_CBGP(jEnv, joNode), pLink);

  return_jni_unlock(jEnv, joLink);
}

// -----[ addPTPLink ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addPTPLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Node;Ljava/lang/String;Ljava/lang/String;Z)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addPTPLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jstring joSrcIface,
   jstring jsDstIface, jboolean jbBidir)
{
  jni_lock(jEnv);

  return_jni_unlock(jEnv, NULL);
}

// -----[ addPTMPLink ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addPTMPLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Subnet;Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addPTMPLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jstring jsIface)
{
  jni_lock(jEnv);

  return_jni_unlock(jEnv, NULL);
}

// -----[ _cbgp_jni_get_link ]---------------------------------------
static int _cbgp_jni_get_link(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  SNetLink * pLink= *((SNetLink **) pItem);
  jobject joLink;

  if ((joLink= cbgp_jni_new_net_Link(pCtx->jEnv,
				     pCtx->joCBGP,
				     pLink)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joLink);
}

// -----[ getLinks ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getLinks
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLinks
  (JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;
  jobject joVector;
  SJNIContext sCtx;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= jni_proxy_get_CBGP(jEnv, joNode);
  if (node_links_for_each(pNode, _cbgp_jni_get_link, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_rt_route ]------------------------------------
static int _cbgp_jni_get_rt_route(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  SNetRouteInfo * pRI= (SNetRouteInfo *) pItem;
  SPrefix sPrefix;
  jobject joRoute;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  if ((joRoute= cbgp_jni_new_IPRoute(pCtx->jEnv, sPrefix, pRI)) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ getRT ]----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getRT
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getRT
  (JNIEnv * jEnv, jobject joNode, jstring jsPrefix)
{
  SNetNode * pNode;
  SPrefix sPrefix;
  jobject joVector;
  jobject joRoute;
  SJNIContext sCtx;
  SNetRouteInfo * pRI;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the prefix */
  if (jsPrefix != NULL) {
    if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
      return_jni_unlock(jEnv, NULL);
  } else {
    sPrefix.uMaskLen= 0;
  }

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  if (jsPrefix == NULL) {

    sCtx.jEnv= jEnv;
    sCtx.joVector= joVector;
    if (rt_for_each(pNode->pRT, _cbgp_jni_get_rt_route, &sCtx) != 0)
      return_jni_unlock(jEnv, NULL);

  } else {

    if (sPrefix.uMaskLen == 32) {
      pRI= rt_find_best(pNode->pRT, sPrefix.tNetwork, NET_ROUTE_ANY);
    } else {
      pRI= rt_find_exact(pNode->pRT, sPrefix, NET_ROUTE_ANY);
    }

    if (pRI != NULL) {
      if ((joRoute= cbgp_jni_new_IPRoute(jEnv, sPrefix, pRI)) == NULL)
	return_jni_unlock(jEnv, NULL);
      cbgp_jni_Vector_add(jEnv, joVector, joRoute);
    }
    
  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ hasProtocol ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    hasProtocol
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_hasProtocol
  (JNIEnv * jEnv, jobject joNode, jstring jsProtocol)
{
  SNetNode * pNode;
  unsigned int uIndex;
  const char * pcProtocol;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  /* Find protocol index */
  pcProtocol= (*jEnv)->GetStringUTFChars(jEnv, jsProtocol, NULL);
    uIndex= 0;
  while (uIndex < NET_PROTOCOL_MAX) {
    if (!strcmp(PROTOCOL_NAMES[uIndex], pcProtocol))
      break;
    uIndex++;
  }
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsProtocol, pcProtocol);

  /* Protocol name unknown */
  if (uIndex >= NET_PROTOCOL_MAX)
    return_jni_unlock(jEnv, JNI_FALSE);

  /* Check if protocol is supported */
  if (node_get_protocol(pNode, uIndex) != NULL)
    return_jni_unlock(jEnv, JNI_TRUE);

  return_jni_unlock(jEnv, JNI_FALSE);
}

// -----[ getProtocols ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getProtocols
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getProtocols
  (JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;
  jobject joVector= NULL;
  int iIndex;
  jstring jsProtocol;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create Vector object */
  joVector= cbgp_jni_new_Vector(jEnv);
  if (joVector == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Populate with identifier of supported protocols */
  for (iIndex= 0; iIndex < NET_PROTOCOL_MAX; iIndex++) {
    if (node_get_protocol(pNode, iIndex)) {
      jsProtocol= cbgp_jni_new_String(jEnv, PROTOCOL_NAMES[iIndex]);
      if (jsProtocol == NULL)
	return_jni_unlock(jEnv, NULL);
      cbgp_jni_Vector_add(jEnv, joVector, jsProtocol);
    }
  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ getBGP ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getBGP
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Router;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getBGP
  (JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;
  SBGPRouter * pRouter;
  SNetProtocol * pProtocol;
  jobject joRouter;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the BGP handler */
  if ((pProtocol= node_get_protocol(pNode, NET_PROTOCOL_BGP)) == NULL)
    return_jni_unlock(jEnv, NULL);
  pRouter= (SBGPRouter *) pProtocol->pHandler;

  /* Create bgp.Router instance */
  if ((joRouter= cbgp_jni_new_bgp_Router(jEnv, jni_proxy_get_CBGP(jEnv, joNode), pRouter)) == NULL)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joRouter);
}

// -----[ addRoute ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addRoute
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addRoute
  (JNIEnv * jEnv, jobject joNode, jstring jsPrefix,
   jstring jsNexthop, jint jiWeight)
{
  SNetNode * pNode;
  SPrefix sPrefix;
  net_addr_t tNextHop;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_address(jEnv, jsNexthop, &tNextHop) != 0)
    return_jni_unlock2(jEnv);

  if (node_rt_add_route(pNode, sPrefix, tNextHop, tNextHop,
			jiWeight, NET_ROUTE_STATIC) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not add route");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}

// -----[ getLatitude ]----------------------------------------------
/**
 *
 */
JNIEXPORT jfloat JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLatitude
(JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, 0);

  return_jni_unlock(jEnv, pNode->fLatitude);
}

// -----[ setLatitude ]----------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setLatitude
(JNIEnv * jEnv, jobject joNode, jfloat jfLatitude)
{
  SNetNode * pNode;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  pNode->fLatitude= jfLatitude;

  jni_unlock(jEnv);
}

// -----[ getLongitude ]---------------------------------------------
/**
 *
 */
JNIEXPORT jfloat JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLongitude
(JNIEnv * jEnv, jobject joNode)
{
  SNetNode * pNode;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, 0);

  return_jni_unlock(jEnv, pNode->fLongitude);
}

// -----[ setLongitude ]---------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setLongitude
(JNIEnv * jEnv, jobject joNode, jfloat jfLongitude)
{
  SNetNode * pNode;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  pNode->fLongitude= jfLongitude;

  jni_unlock(jEnv);
}

// -----[ loadTraffic ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    loadTraffic
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_loadTraffic
  (JNIEnv * jEnv, jobject joNode, jstring jsFileName)
{
  SNetNode * pNode;
  const char * pcFileName;
  uint8_t tOptions= 0;
  int iResult;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (SNetNode*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  /* Load Netflow from file */
  pcFileName= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  iResult= node_load_netflow(pNode, pcFileName, tOptions);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, pcFileName);
  if (iResult != NET_SUCCESS) {
    cbgp_jni_throw_CBGPException(jEnv, "could not load Netflow");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}
