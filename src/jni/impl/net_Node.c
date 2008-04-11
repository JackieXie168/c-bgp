// ==================================================================
// @(#)net_Node.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/04/2006
// $Id: net_Node.c,v 1.17 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <jni_md.h>
#include <jni.h>
#include <jni/exceptions.h>
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
			      net_node_t * node)
{
  jobject joNode;
  jobject joAddress;

  /* Java proxy object already existing ? */
  joNode= jni_proxy_get(jEnv, node);
  if (joNode != NULL)
    return joNode;

  /* Convert node attributes to Java objects */
  joAddress= cbgp_jni_new_IPAddress(jEnv, node->addr);

  /* Check that the conversion was successful */
  if (joAddress == NULL)
    return NULL;

  /* Create new Node object */
  if ((joNode= cbgp_jni_new(jEnv, CLASS_Node, CONSTR_Node,
			    joCBGP,
			    joAddress)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joNode, node);

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
  net_node_t * node;
  jstring jsName= NULL;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the name */
  if (node->name != NULL)
    jsName= cbgp_jni_new_String(jEnv, node->name);

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
  net_node_t * node;
  const char * name;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  /* Set the name */
  if (jsName != NULL) {
    name= (*jEnv)->GetStringUTFChars(jEnv, jsName, NULL);
    node_set_name(node, name);
    (*jEnv)->ReleaseStringUTFChars(jEnv, jsName, name);
  } else
    node_set_name(node, NULL);

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
  net_node_t * pNode;
  jobject joIPTrace;
  SNetDest sDest;
  ip_trace_t * trace;
  net_error_t error;
  uint8_t options;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the destination */
  if (ip_jstring_to_dest(jEnv, jsDestination, &sDest) != 0)
    return_jni_unlock(jEnv, NULL);

  /* "Any" destination not allowed here */
  if (sDest.tType == NET_DEST_ANY) {
    throw_CBGPException(jEnv, "invalid destination (*)");
    return_jni_unlock(jEnv, NULL);
  }

  /* Trace the IP-level route */
  icmp_options_init(&options);
  icmp_options_add(&options, ICMP_RR_OPTION_DELAY);
  icmp_options_add(&options, ICMP_RR_OPTION_WEIGHT);
  if (sDest.tType == NET_DEST_PREFIX)
    icmp_options_add(&options, ICMP_RR_OPTION_ALT_DEST);
  error= icmp_record_route(pNode, sDest.uDest.tAddr,
			   &sDest.uDest.sPrefix,
			   255, options, &trace, 0);

  /* Convert to an IPTrace object */
  joIPTrace= cbgp_jni_new_IPTrace(jEnv, pNode, sDest.uDest.tAddr, trace);

  ip_trace_destroy(&trace);

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
  net_node_t * pNode;
  jobject joIPTrace;
  SNetDest sDest;
  ip_trace_t * trace;
  net_error_t error;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination */
  if (ip_jstring_to_address(jEnv, jsDestination, &sDest.uDest.tAddr) != 0)
    return_jni_unlock(jEnv, NULL);
  sDest.tType= NET_DEST_ADDRESS;

  /* Trace the IP-level route */
  error= icmp_trace_route(NULL, pNode, NET_ADDR_ANY,
			  sDest.uDest.tAddr, 0, &trace);

  /* Convert to an IPTrace object */
  joIPTrace= cbgp_jni_new_IPTrace(jEnv, pNode, sDest.uDest.tAddr, trace);

  ip_trace_destroy(&trace);

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
  net_node_t * pNode;
  jobject joVector= NULL;
  SJNIContext sCtx;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
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
  net_node_t * pNode, *pNodeDst;
  net_iface_t * pIface;
  jobject joLink;
  int iResult;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the destination */
  pNodeDst= (net_node_t*) jni_proxy_lookup(jEnv, joDst);
  if (pNodeDst == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Add link */
  iResult= net_link_create_rtr(pNode, pNodeDst,
			       (jbBidir==JNI_TRUE) ? BIDIR : UNIDIR,
			       &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  joLink= cbgp_jni_new_net_Link(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
				pIface);

  return_jni_unlock(jEnv, joLink);
}

// -----[ addPTPLink ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addPTPLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Node;Ljava/lang/String;Ljava/lang/String;Z)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addPTPLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jstring jsSrcIface,
   jstring jsDstIface, jboolean jbBidir)
{
  net_node_t * pNode;
  net_node_t * pNodeDst;
  int iResult;
  net_iface_t * pIface;
  net_iface_id_t tSrcIfaceID;
  net_iface_id_t tDstIfaceID;
  jobject joIface;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the destination */
  pNodeDst= (net_node_t*) jni_proxy_lookup(jEnv, joDst);
  if (pNodeDst == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the source's interface ID */
  if (ip_jstring_to_prefix(jEnv, jsSrcIface, &tSrcIfaceID) != 0)
    return_jni_unlock(jEnv, NULL);

  /* Get the destination's interface ID */
  if (ip_jstring_to_prefix(jEnv, jsDstIface, &tDstIfaceID) != 0)
    return_jni_unlock(jEnv, NULL);

  /* Add link */
  iResult= net_link_create_ptp(pNode, tSrcIfaceID,
			       pNodeDst, tDstIfaceID,
			       (jbBidir==JNI_TRUE) ? BIDIR : UNIDIR,
			       &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  joIface= cbgp_jni_new_net_Link(jEnv,
				 NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
				 pIface);

  return_jni_unlock(jEnv, joIface);
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
  net_node_t * pNode;
  net_subnet_t * pSubnet;
  net_iface_t * pIface;
  jobject joIface;
  int iResult;
  net_addr_t tIfaceID;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the subnet */
  pSubnet= (net_subnet_t *) jni_proxy_lookup(jEnv, joDst);
  if (pSubnet == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the interface ID */
  if (ip_jstring_to_address(jEnv, jsIface, &tIfaceID) != 0)
    return_jni_unlock(jEnv, NULL);  

  /* Add link */
  iResult= net_link_create_ptmp(pNode, pSubnet, tIfaceID, &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  joIface= cbgp_jni_new_net_Link(jEnv,
				 NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
				 pIface);
  return_jni_unlock(jEnv, joIface);
}

// -----[ _cbgp_jni_get_link ]---------------------------------------
static int _cbgp_jni_get_link(void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  net_iface_t * pLink= *((net_iface_t **) pItem);
  jobject joLink;

  if (!net_iface_is_connected(pLink))
    return 0;

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
  net_node_t * pNode;
  jobject joVector;
  SJNIContext sCtx;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/;
  if (node_links_for_each(pNode, _cbgp_jni_get_link, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_rt_route ]------------------------------------
static int _cbgp_jni_get_rt_route(uint32_t uKey, uint8_t uKeyLen,
				  void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  rt_info_t * rtinfo= (rt_info_t *) pItem;
  SPrefix sPrefix;
  jobject joRoute;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  if ((joRoute= cbgp_jni_new_IPRoute(pCtx->jEnv, sPrefix, rtinfo)) == NULL)
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
  (JNIEnv * jEnv, jobject joNode, jstring jsDest)
{
  net_node_t * node;
  jobject joVector;
  SJNIContext sCtx;
  rt_info_t * rtinfo;
  SNetDest sDest;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination specifier (*|address|prefix) */
  if (jsDest != NULL) {
    if (ip_jstring_to_dest(jEnv, jsDest, &sDest) < 0)
      return_jni_unlock(jEnv, NULL);
  } else
    sDest.tType= NET_ADDR_ANY;

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Prepare JNI context */
  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;

  switch (sDest.tType) {
  case NET_DEST_ANY:
    if (rt_for_each(node->rt, _cbgp_jni_get_rt_route, &sCtx) != 0)
      return_jni_unlock(jEnv, NULL);
    break;

  case NET_DEST_ADDRESS:
    rtinfo= rt_find_best(node->rt, sDest.uDest.tAddr, NET_ROUTE_ANY);

    if (rtinfo != NULL)
      if (_cbgp_jni_get_rt_route(sDest.uDest.tAddr, 32, rtinfo, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
    break;

  case NET_DEST_PREFIX:
    rtinfo= rt_find_exact(node->rt, sDest.uDest.sPrefix, NET_ROUTE_ANY);
    if (rtinfo != NULL)
      if (_cbgp_jni_get_rt_route(sDest.uDest.sPrefix.tNetwork,
				 sDest.uDest.sPrefix.uMaskLen,
				 rtinfo, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
    break;
    
  case NET_DEST_INVALID:
    throw_CBGPException(jEnv, "invalid destination");
    break;

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
  net_node_t * pNode;
  const char * pcProtocol;
  net_protocol_id_t tProtoID;
  net_error_t error;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  /* Find protocol index */
  pcProtocol= (*jEnv)->GetStringUTFChars(jEnv, jsProtocol, NULL);
  error= net_str2protocol(pcProtocol, &tProtoID);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsProtocol, pcProtocol);

  /* Protocol name unknown */
  if (error != ESUCCESS)
    return_jni_unlock(jEnv, JNI_FALSE);

  /* Check if protocol is supported */
  if (node_get_protocol(pNode, tProtoID) != NULL)
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
  net_node_t * pNode;
  jobject joVector= NULL;
  net_protocol_id_t tProtoID;
  jstring jsProtocol;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create Vector object */
  joVector= cbgp_jni_new_Vector(jEnv);
  if (joVector == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Populate with identifier of supported protocols */
  for (tProtoID= 0; tProtoID < NET_PROTOCOL_MAX; tProtoID++) {
    if (node_get_protocol(pNode, tProtoID)) {
      jsProtocol= cbgp_jni_new_String(jEnv, net_protocol2str(tProtoID));
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
  net_node_t * node;
  bgp_router_t * router;
  net_protocol_t * protocol;
  jobject joRouter;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the BGP handler */
  if ((protocol= node_get_protocol(node, NET_PROTOCOL_BGP)) == NULL)
    return_jni_unlock(jEnv, NULL);
  router= (bgp_router_t *) protocol->pHandler;

  /* Create bgp.Router instance */
  if ((joRouter= cbgp_jni_new_bgp_Router(jEnv,
					 NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
					 router)) == NULL)
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
  net_node_t * pNode;
  SPrefix sPrefix;
  net_addr_t tNextHop;
  net_iface_id_t tIfaceID;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &sPrefix) != 0)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_address(jEnv, jsNexthop, &tNextHop) != 0)
    return_jni_unlock2(jEnv);

  tIfaceID.tNetwork= tNextHop;
  tIfaceID.uMaskLen= 32;
  if (node_rt_add_route(pNode, sPrefix, tIfaceID,
			tNextHop, jiWeight, NET_ROUTE_STATIC) != 0) {
    throw_CBGPException(jEnv, "could not add route");
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
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, 0);

  return_jni_unlock(jEnv, node->coord.latitude);
}

// -----[ setLatitude ]----------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setLatitude
(JNIEnv * jEnv, jobject joNode, jfloat jfLatitude)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  node->coord.latitude= jfLatitude;

  jni_unlock(jEnv);
}

// -----[ getLongitude ]---------------------------------------------
/**
 *
 */
JNIEXPORT jfloat JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLongitude
(JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, 0);

  return_jni_unlock(jEnv, node->coord.longitude);
}

// -----[ setLongitude ]---------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setLongitude
(JNIEnv * jEnv, jobject joNode, jfloat jfLongitude)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  node->coord.longitude= jfLongitude;

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
  net_node_t * pNode;
  const char * pcFileName;
  uint8_t tOptions= 0;
  int iResult;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock2(jEnv);

  /* Load Netflow from file */
  pcFileName= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  iResult= node_load_netflow(pNode, pcFileName, tOptions);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, pcFileName);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not load Netflow");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}
