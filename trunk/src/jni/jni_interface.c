// ==================================================================
// @(#)jni_interface.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/10/2004
// @lastdate 07/04/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/str_util.h>

#include <jni.h>
#include <string.h>
#include <assert.h>

#include <jni/be_ac_ucl_ingi_cbgp_CBGP.h>
#include <jni/jni_util.h>

#include <net/network.h>
#include <net/protocol.h>
#include <net/prefix.h>
#include <net/igp.h>
#include <net/routing.h>

#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/domain.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/filter.h>
#include <bgp/filter_t.h>

#include <sim/simulator.h>

#include <cli/common.h>

#include <libgds/log.h>
#include <libgds/cli_ctx.h>

/* BQU: TO BE FIXED!!!
SFilter * pFilter;
SFilterMatcher * pMatcher;
SFilterAction * pAction;
*/

// -----[ init ]-----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_init
  (JNIEnv * env, jobject obj, jstring file_log)
{
  const jbyte * cFileLog;

  cFileLog = (*env)->GetStringUTFChars(env, file_log, NULL);
  
  //log_set_level(pMainLog, LOG_LEVEL_WARNING);
  
  log_set_level(pMainLog, LOG_LEVEL_EVERYTHING);
  if (strcmp(cFileLog, ""))
    log_set_file(pMainLog, (char *)cFileLog);
  else
    log_set_stream(pMainLog, stderr);


  (*env)->ReleaseStringUTFChars(env, file_log, cFileLog);

  simulator_init(); 
}

// -----[ destroy ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_destroy
  (JNIEnv * env, jobject obj)
{
  simulator_done();
}

// -----[ netAddNode ]-----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeAdd
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddNode
  (JNIEnv * env, jobject obj, jstring jsAddr)
{
  SNetNode * pNode; 
  SNetwork * pNetwork= network_get();
  net_addr_t tNetAddr;

  if (ip_jstring_to_address(env, jsAddr, &tNetAddr) != 0)
    return;
  if ((pNode= node_create(tNetAddr)) == NULL) {
    cbgp_jni_throw_CBGPException(env, "node could not be created");
    return;
  }

  if (network_add_node(pNetwork, pNode) != 0) {
    cbgp_jni_throw_CBGPException(env, "node already exists");
    return;
  }
}

// -----[ netAddLink ]-----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeLinkAdd
 * Signature: (II)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddLink
  (JNIEnv *env, jobject obj, jstring jsSrcAddr, jstring jsDstAddr, jint jiWeight)
{
  SNetNode * pNodeSrc, * pNodeDst;

  if ((pNodeSrc= cbgp_jni_net_node_from_string(env, jsSrcAddr)) == NULL)
    return;
  if ((pNodeDst= cbgp_jni_net_node_from_string(env, jsDstAddr)) == NULL)
    return;

  if (node_add_link(pNodeSrc, pNodeDst, jiWeight, 1) != 0) {
    cbgp_jni_throw_CBGPException(env, "link already exists");
    return;
  }
}

// -----[ netLinkWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeLinkWeight
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netLinkWeight
  (JNIEnv * env, jobject obj, jstring jsSrcAddr, jstring jsDstAddr, jint jiWeight)
{
  SNetLink * pLink1, * pLink2;

  pLink1= cbgp_jni_net_link_from_string(env, jsSrcAddr, jsDstAddr);
  if (pLink1 == NULL)
    return;

  pLink2= cbgp_jni_net_link_from_string(env, jsDstAddr, jsSrcAddr);
  if (pLink2 == NULL)
    return;

  link_set_igp_weight(pLink1, jiWeight);
  link_set_igp_weight(pLink2, jiWeight);
}

// -----[ netLinkUp ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeLinkUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netLinkUp
  (JNIEnv * env, jobject obj, jstring jsSrcAddr, jstring jsDstAddr, jboolean bUp)
{
  SNetLink * pLink1, * pLink2;

  pLink1= cbgp_jni_net_link_from_string(env, jsSrcAddr, jsDstAddr);
  if (pLink1 == NULL)
    return;

  pLink2= cbgp_jni_net_link_from_string(env, jsDstAddr, jsSrcAddr);
  if (pLink2 == NULL)
    return;

  link_set_state(pLink1, NET_LINK_FLAG_UP, (bUp == JNI_TRUE)?1:0);
  link_set_state(pLink2, NET_LINK_FLAG_UP, (bUp == JNI_TRUE)?1:0);
}
  
// -----[ netNodeSpfPrefix ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeSpfPrefix
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netNodeSpfPrefix
  (JNIEnv * env, jobject obj, jstring jsAddr, jstring jsPrefix)
{
  SNetwork * pNetwork= network_get();
  SPrefix sPrefix;
  SNetNode * pNode;

  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return;
  if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
    return;

  // Compute the SPF from the node towards all the nodes in the prefix
  if (igp_compute_prefix(pNetwork, pNode, sPrefix) != 0) {
    cbgp_jni_throw_CBGPException(env, "could not compute IGP paths");
    return;
  }
}

// -----[ nodeInterfaceAdd ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeInterfaceAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_nodeInterfaceAdd
  (JNIEnv * env, jobject obj, jstring net_addr_id, jstring net_addr_int, jstring mask)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetInterface * pInterface; 
  net_addr_t iNetAddrId, iNetAddrInt;
  SPrefix * pMask = MALLOC(sizeof(SPrefix));
 
  if (ip_jstring_to_address(env, net_addr_id, &iNetAddrId) != 0)
     return -1;
  if ((pNode= network_find_node(pNetwork, iNetAddrId)) != NULL)
    return -1;
  if (ip_jstring_to_address(env, net_addr_int, &iNetAddrInt) != 0)
    return -1;
  if (ip_jstring_to_prefix(env, mask, pMask) != 0)
    return -1;
  
  pInterface= node_interface_create();
  pInterface->tAddr= iNetAddrInt;
  pInterface->tMask= pMask;

  return node_interface_add(pNode, pInterface);
}*/

// -----[ netNodeRouteAdd ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeRouteAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netNodeRouteAdd
  (JNIEnv * env, jobject obj, jstring jsAddr, jstring jsPrefix,
   jstring jsNexthop, jint jiWeight)
{
  SNetNode * pNode;
  SPrefix sPrefix;
  net_addr_t tNextHop;

  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return;
  if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
    return;
  if (ip_jstring_to_address(env, jsNexthop, &tNextHop) != 0)
    return;

  if (node_rt_add_route(pNode, sPrefix, tNextHop,
			jiWeight, NET_ROUTE_STATIC) != 0) {
    cbgp_jni_throw_CBGPException(env, "could not add route");
    return;
  }
}

// -----[ cbgp_jni_get_link ]----------------------------------------
int cbgp_jni_get_link(void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  SNetLink * pLink= *((SNetLink **) pItem);
  jobject joLink;

  if ((joLink= cbgp_jni_new_Link(pCtx->jEnv, pLink)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joLink);
}

// -----[ netNodeGetLinks ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeShowLinks
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netNodeGetLinks
  (JNIEnv * env, jobject obj, jstring jsAddr)
{
  SNetNode * pNode;
  jobject joVector;
  SRouteDumpCtx sCtx;

  /* Get the node */
  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return NULL;

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
    return NULL;

  sCtx.joVector= joVector;
  sCtx.jEnv= env;
  if (node_links_for_each(pNode, cbgp_jni_get_link, &sCtx) != 0)
    return NULL;
  
  return joVector;
}

// -----[ cbgp_jni_get_rt_route ]------------------------------------
int cbgp_jni_get_rt_route(uint32_t uKey, uint8_t uKeyLen,
			  void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  SNetRouteInfo * pRI= (SNetRouteInfo *) pItem;
  SPrefix sPrefix;
  jobject joRoute;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  if ((joRoute= cbgp_jni_new_IPRoute(pCtx->jEnv, sPrefix, pRI)) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ netNodeGetRT ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeShowRT
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netNodeGetRT
  (JNIEnv * env, jobject obj, jstring jsNodeAddr, jstring jsPrefix)
{
  SNetNode * pNode;
  SPrefix sPrefix;
  jobject joVector;
  jobject joRoute;
  SRouteDumpCtx sCtx;
  SNetRouteInfo * pRI;

  /* Get the node */
  if ((pNode= cbgp_jni_net_node_from_string(env, jsNodeAddr)) == NULL)
    return  NULL;

  /* Convert the prefix */
  if (jsPrefix != NULL) {
    if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
      return NULL;
  } else {
    sPrefix.uMaskLen= 0;
  }

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
    return NULL;

  if (jsPrefix == NULL) {

    sCtx.jEnv= env;
    sCtx.joVector= joVector;
    if (rt_for_each(pNode->pRT, cbgp_jni_get_rt_route, &sCtx) != 0)
      return NULL;

  } else {

    if (sPrefix.uMaskLen == 32) {
      pRI= rt_find_best(pNode->pRT, sPrefix.tNetwork, NET_ROUTE_ANY);
    } else {
      pRI= rt_find_exact(pNode->pRT, sPrefix, NET_ROUTE_ANY);
    }

    if (pRI != NULL) {
      if ((joRoute= cbgp_jni_new_IPRoute(env, sPrefix, pRI)) == NULL)
	return NULL;
      cbgp_jni_Vector_add(env, joVector, joRoute);
    }
    
  }

  return joVector;
}

// -----[ netNodeRecordRoute ]---------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netNodeRecordRoute
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPTrace;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netNodeRecordRoute
  (JNIEnv * env, jobject obj, jstring jsNodeAddr, jstring jsDstAddr)
{
  SNetNode * pNode;
  jobject joIPTrace;
  SNetDest sDest;
  SNetPath * pPath;
  net_link_delay_t tDelay;
  net_link_delay_t tWeight;
  int iStatus;

  /* Get the node */
  if ((pNode= cbgp_jni_net_node_from_string(env, jsNodeAddr)) == NULL)
    return  NULL;

  /* Convert the destination */
  if (ip_jstring_to_address(env, jsDstAddr, &sDest.uDest.tAddr) != 0)
    return NULL;
  sDest.tType= NET_DEST_ADDRESS;

  /* Trace the IP-level route */
  iStatus= node_record_route(pNode, sDest, &pPath, &tDelay, &tWeight);

  /* Convert to an IPTrace object */
  joIPTrace= cbgp_jni_new_IPTrace(env, pNode->tAddr, sDest.uDest.tAddr,
				  pPath, iStatus, tDelay, tWeight);    

  net_path_destroy(&pPath);

  return joIPTrace;
}


// -----[ bgpAddRouter ]---------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 *
 * Note: 'net_addr' must be non-NULL. 'name' may be NULL.
 * Returns: -1 on error and 0 on success.
 */
JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpAddRouter
  (JNIEnv * env, jobject obj, jstring jsName, jstring jsAddr, jint jiASNumber)
{
  SNetNode * pNode;
  SBGPRouter * pRouter;
  const jbyte * pcName;

  /* Try to find related router. */
  if ((pNode= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return -1;

  /* Create BGP router, and register. */
  pRouter= bgp_router_create(jiASNumber, pNode, 0);
  if (node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter, 
			     (FNetNodeHandlerDestroy) bgp_router_destroy,
			     bgp_router_handle_message)) {
    bgp_router_destroy(&pRouter);
    cbgp_jni_throw_CBGPException(env, "Node already supports BGP");
    return -1;
  }

  /* Add the given name if non-NULL. */
  if (jsName != NULL) {
    pcName = (*env)->GetStringUTFChars(env, jsName, NULL);
    bgp_router_set_name(pRouter, str_create((char *) pcName));
    (*env)->ReleaseStringUTFChars(env, jsName, pcName);
  }

  return 0;
}

// -----[ bgpRouterAddNetwork ]--------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterNetworkAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterAddNetwork
  (JNIEnv * env, jobject obj, jstring jsAddr, jstring jsPrefix)
{
  SBGPRouter * pRouter;
  SPrefix sPrefix;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsAddr)) == NULL)
    return;
  if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
    return;

  if (bgp_router_add_network(pRouter, sPrefix) != 0) {
    cbgp_jni_throw_CBGPException(env, "coud not add network");
    return;
  }
}

// -----[ bgpRouterAddPeer ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterNeighborAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterAddPeer
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr, jint jiASNumber)
{
  SBGPRouter * pRouter;
  net_addr_t tPeerAddr;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return;
  if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
    return;
  
  if (bgp_router_add_peer(pRouter, jiASNumber, tPeerAddr, 0) != 0) {
    cbgp_jni_throw_CBGPException(env, "could not add peer");
    return;
  }
}

// -----[ bgpRouterPeerNextHopSelf ]---------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterNeighborNextHopSelf
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerNextHopSelf
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
}

// -----[ bgpRouterPeerReflectorClient ]-----------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterPeerReflectorClient
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerReflectorClient
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  peer_flag_set(pPeer, PEER_FLAG_RR_CLIENT, 1);
}


// -----[ bgpRouterPeerVirtual ]-------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterPeerVirtual
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerVirtual
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
  peer_flag_set(pPeer, PEER_FLAG_SOFT_RESTART, 1);
}

// -----[ bgpRouterPeerUp ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterNeighborUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerUp
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr, jboolean bUp)
{
  SBGPPeer * pPeer;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  if (bUp == JNI_TRUE) {
    if (bgp_peer_open_session(pPeer) != 0)
      cbgp_jni_throw_CBGPException(env, "could not open session");
  } else {
    if (bgp_peer_close_session(pPeer) != 0)
      cbgp_jni_throw_CBGPException(env, "could not close session");
  }
}

// -----[ bgpRouterPeerRecv ]----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterPeerRecv
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterPeerRecv
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr, jstring jsMesg)
{
  SBGPRouter * pRouter;
  SBGPPeer * pPeer;
  const jbyte * cMesg;
  SBGPMsg * pMsg;

  pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr);
  if (pRouter == NULL)
    return;

  pPeer= cbgp_jni_bgp_peer_from_string(env, jsRouterAddr, jsPeerAddr);
  if (pPeer == NULL)
    return;

  /* Build a message from the MRT-record */
  cMesg= (*env)->GetStringUTFChars(env, jsMesg, NULL);
  if ((pMsg= mrtd_msg_from_line(pRouter, pPeer, (char *) cMesg)) != NULL) {
    if (peer_handle_message(pPeer, pMsg) != 0)
      cbgp_jni_throw_CBGPException(env, "could not handle message");
  } else {
    cbgp_jni_throw_CBGPException(env, "could not understand MRT message");
  }
  (*env)->ReleaseStringUTFChars(env, jsMesg, cMesg);
}

// -----[ bgpRouterRescan ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterRescan
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterRescan
  (JNIEnv * env, jobject obj, jstring jsAddr)
{
  SBGPRouter * pRouter;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsAddr)) == NULL)
    return;
 
  if (bgp_router_scan_rib(pRouter) != 0)
    cbgp_jni_throw_CBGPException(env, "could not rescan router");
}

// -----[ cbgp_jni_get_peer ]----------------------------------------
int cbgp_jni_get_peer(void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  jobject joPeer;

  if ((joPeer= cbgp_jni_new_BGPPeer(pCtx->jEnv, *((SPeer **) pItem))) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joPeer);
}

// -----[ bgpRouterGetPeers ]----------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterGetPeers
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterGetPeers
  (JNIEnv * env, jobject obj, jstring jsRouterAddr)
{
  SBGPRouter * pRouter;
  jobject joVector;
  SRouteDumpCtx sCtx;

  /* Get the router instance */
  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return NULL;

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
    return NULL;

  sCtx.joVector= joVector;
  sCtx.jEnv= env;
  if (bgp_router_peers_for_each(pRouter, cbgp_jni_get_peer, &sCtx) != 0)
    return NULL;

  return joVector;
}


// -----[ cbgp_jni_get_rib_route ]-----------------------------------
int cbgp_jni_get_rib_route(uint32_t uKey, uint8_t uKeyLen,
			   void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  jobject joRoute;

  if ((joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, (SRoute *) pItem)) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ bgpRouterGetRib ]------------------------------------------
/**
 * Class    : CBGP
 * Method   : bgpRouterGetRib
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 *
 * This function creates an array of BGPRoute objects corresponding
 * with the content of the given router's RIB.
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterGetRib
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPrefix)
{
  SBGPRouter * pRouter;
  jobject joVector;
  SPrefix sPrefix;
  SRoute * pRoute;
  jobject joRoute;

  /* Get the router instance */
  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return NULL;

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
    return NULL;

  if (jsPrefix == NULL) {

    /* For each route in the RIB, create a new BGPRoute object and add
       it to the Vector object */
    SRouteDumpCtx sCtx;
    sCtx.pRouter= pRouter;
    sCtx.joVector= joVector;
    sCtx.jEnv= env;
    if (rib_for_each(pRouter->pLocRIB, cbgp_jni_get_rib_route, &sCtx) != 0)
      return NULL;

  } else {

    /* Convert the prefix */
    if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
      return NULL;

    pRoute= NULL;
    if (sPrefix.uMaskLen == 32)
      pRoute= rib_find_best(pRouter->pLocRIB, sPrefix);
    else
      pRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);

    if (pRoute != NULL) {
      if ((joRoute= cbgp_jni_new_BGPRoute(env, pRoute)) == NULL)
	return NULL;
      cbgp_jni_Vector_add(env, joVector, joRoute);
    }

  }

  return joVector;
}

// -----[ bgpRouterGetAdjRib ]---------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpRouterShowRibIn
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterGetAdjRib
  (JNIEnv * env, jobject obj, jstring jsRouterAddr,
   jstring jsPeerAddr, jstring jsPrefix, jboolean bIn)
{
  SBGPRouter * pRouter;
  jobject joVector;
  jobject joRoute;
  SPrefix sPrefix;
  net_addr_t tPeerAddr;
  int iIndex;
  SPeer * pPeer= NULL;
  SRoute * pRoute;

  /* Get the router instance */
  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return NULL;

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(env)) == NULL)
    return NULL;

  /* Convert the peer address */
  if (jsPeerAddr != NULL) {
    if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
      return NULL;
    if ((pPeer= bgp_router_find_peer(pRouter, tPeerAddr)) == NULL)
      return NULL;
  }

  /* Convert prefix */
  if (jsPrefix == NULL) {
    
    /* Add routes into the Vector object */
    if (pPeer == NULL) {
      
      for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
	pPeer= (SPeer *) pRouter->pPeers->data[iIndex];
	
	/* For each route in the RIB, create a new BGPRoute object and add
	   it to the Vector object */
	SRouteDumpCtx sCtx;
	sCtx.pRouter= pRouter;
	sCtx.joVector= joVector;
	sCtx.jEnv= env;
	if (bIn == JNI_TRUE) {
	  if (rib_for_each(pPeer->pAdjRIBIn, cbgp_jni_get_rib_route, &sCtx) != 0)
	    return NULL;
	} else {
	  if (rib_for_each(pPeer->pAdjRIBOut, cbgp_jni_get_rib_route, &sCtx) != 0)
	    return NULL;
	}
	
      }
      
    } else {
      
      /* For each route in the RIB, create a new BGPRoute object and add
	 it to the Vector object */
      SRouteDumpCtx sCtx;
      sCtx.pRouter= pRouter;
      sCtx.joVector= joVector;
      sCtx.jEnv= env;
      if (bIn == JNI_TRUE) {
	if (rib_for_each(pPeer->pAdjRIBIn, cbgp_jni_get_rib_route, &sCtx) != 0)
	  return NULL;
      } else {
	if (rib_for_each(pPeer->pAdjRIBOut, cbgp_jni_get_rib_route, &sCtx) != 0)
	  return NULL;
      }

    }
    
  } else {

    /* Convert the prefix */
    if (ip_jstring_to_prefix(env, jsPrefix, &sPrefix) != 0)
      return NULL;
    
    if (pPeer == NULL) {
      
      for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
	pPeer= (SPeer *) pRouter->pPeers->data[iIndex];

	pRoute= NULL;
	if (sPrefix.uMaskLen == 32) {
	  if (bIn == JNI_TRUE) {
	    pRoute= rib_find_best(pPeer->pAdjRIBIn, sPrefix);
	  } else {
	    pRoute= rib_find_best(pPeer->pAdjRIBOut, sPrefix);
	  }
	} else {
	  if (bIn == JNI_TRUE) {
	    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
	  } else {
	    pRoute= rib_find_exact(pPeer->pAdjRIBOut, sPrefix);
	  }
	}
	
	if (pRoute != NULL) {
	  if ((joRoute= cbgp_jni_new_BGPRoute(env, pRoute)) == NULL)
	    return NULL;
	  cbgp_jni_Vector_add(env, joVector, joRoute);
	}
      }

    } else {

      pRoute= NULL;
      if (sPrefix.uMaskLen == 32) {
	if (bIn == JNI_TRUE) {
	  pRoute= rib_find_best(pPeer->pAdjRIBIn, sPrefix);
	} else {
	  pRoute= rib_find_best(pPeer->pAdjRIBOut, sPrefix);
	}
      } else {
	if (bIn == JNI_TRUE) {
	  pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
	} else {
	  pRoute= rib_find_exact(pPeer->pAdjRIBOut, sPrefix);
	}
      }
      
      if (pRoute != NULL) {
	if ((joRoute= cbgp_jni_new_BGPRoute(env, pRoute)) == NULL)
	  return NULL;
	cbgp_jni_Vector_add(env, joVector, joRoute);
      }

    }

  }

  return joVector;
}

// -----[ bgpRouterLoadRib ]-----------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpRouterLoadRib
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpRouterLoadRib
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsFileName)
{
  SBGPRouter * pRouter;
  const jbyte * cFileName;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return;

  cFileName= (*env)->GetStringUTFChars(env, jsFileName, NULL);
  if (bgp_router_load_rib((char *) cFileName, pRouter) != 0)
    cbgp_jni_throw_CBGPException(env, "could not load RIB");
  (*env)->ReleaseStringUTFChars(env, jsFileName, cFileName);
}

// -----[ bgpDomainRescan ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpDomainRescan
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpDomainRescan
  (JNIEnv * env, jobject obj, jint jiASNumber)
{
  if (!exists_bgp_domain((uint32_t) jiASNumber)) {
    cbgp_jni_throw_CBGPException(env, "domain does not exist");
    return;
  }

  if (bgp_domain_rescan(get_bgp_domain((uint32_t) jiASNumber)) != 0)
    cbgp_jni_throw_CBGPException(env, "could not rescan domain");
}

// -----[ bgpFilterInit ]--------------------------------------------
/*
 * Class:     CBGP
 * Method:    bgpFilterInit
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterInit
  (JNIEnv * env, jobject obj, jstring jsRouterAddr, jstring jsPeerAddr, 
   jstring type)
{
  SBGPRouter * pRouter;
  SPeer * pPeer;
  net_addr_t tPeerAddr;

  int ret =0;
  const jbyte * cType;

  if ((pRouter= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return -1;
  if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
    return -1;
  
  if ((pPeer= bgp_router_find_peer(pRouter, tPeerAddr)) == NULL)
    return -1;

  cType = (*env)->GetStringUTFChars(env, type, NULL);
  if (strcmp(cType, FILTER_IN) == 0) { 
    pFilter = filter_create();
    peer_set_in_filter(pPeer, pFilter);
  } else if (strcmp(cType, FILTER_OUT) == 0) {
    pFilter = filter_create();
    peer_set_out_filter(pPeer, pFilter);
  } else 
    ret = 1;

  (*env)->ReleaseStringUTFChars(env, type, cType);
  return ret;

}
*/

// -----[ bgpFilterMatchPrefixIn ]-----------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterMatchPrefix
 * Signature: (Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterMatchPrefixIn
  (JNIEnv * env, jobject obj, jstring prefix)
{
  const jbyte * cPrefix;
  char * pcEndPtr;
  SPrefix Prefix;

  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", cPrefix);
  }
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  pMatcher = filter_match_prefix_in(Prefix);
  return 0;
}
*/

/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterMatchPrefix
 * Signature: (Ljava/lang/String;)I
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterMatchPrefixIs
  (JNIEnv * env, jobject obj, jstring prefix)
{
  const jbyte * cPrefix;
  char * pcEndPtr;
  SPrefix Prefix;

  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", cPrefix);
  }
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  pMatcher = filter_match_prefix_equals(Prefix);
  return 0;
}
*/

/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterAction
 * Signature: (ILjava/lang/String;)I
 *
 */
/* BQU: TO BE FIXED!!!

#define ACTION_PERMIT	    0x01
#define ACTION_DENY	    0x02
#define ACTION_LOCPREF	    0x03
#define ACTION_ADD_COMM	    0x04
#define ACTION_PATH_PREPEND 0x05
JNIEXPORT jint JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterAction
  (JNIEnv * env, jobject obj, jint type, jstring value)
{

  const jbyte * cValue;
  uint32_t iValue;

  switch (type) {
    case ACTION_PERMIT:
      pAction = filter_action_accept();
      break;
    case ACTION_DENY:
      pAction = filter_action_deny();
      break;
    case ACTION_LOCPREF:
      cValue = (*env)->GetStringUTFChars(env, value, NULL);
      iValue = atoi(cValue);
      (*env)->ReleaseStringUTFChars(env, value, cValue);
      pAction = filter_action_pref_set(iValue);
      break;
    case ACTION_ADD_COMM:
      cValue = (*env)->GetStringUTFChars(env, value, NULL);
      iValue = atoi(cValue);
      (*env)->ReleaseStringUTFChars(env, value, cValue);
      pAction = filter_action_comm_append(iValue);
      break;
    case ACTION_PATH_PREPEND:
      cValue = (*env)->GetStringUTFChars(env, value, NULL);
      iValue = atoi(cValue);
      (*env)->ReleaseStringUTFChars(env, value, cValue);
      pAction = filter_action_path_prepend(iValue);
      break;
    default:
      return 1;
  }
  filter_add_rule(pFilter, pMatcher, pAction);
  return 0;
}
*/

/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpFilterFinalize
 * Signature: ()V
 */
/* BQU: TO BE FIXED!!!

JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpFilterFinalize
  (JNIEnv *env , jobject obj)
{
  pFilter= NULL;
  pMatcher= NULL;
  pAction= NULL;
}
*/

// -----[ simRun ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simRun
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simRun
  (JNIEnv * env, jobject obj)
{
  if (simulator_run() != 0)
    cbgp_jni_throw_CBGPException(env, "simulation error");
}

// -----[ runCmd ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runCmd
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runCmd
  (JNIEnv * env, jobject obj, jstring jsCommand)
{
  const jbyte * cCommand;

  if (jsCommand == NULL)
    return;

  cCommand= (*env)->GetStringUTFChars(env, jsCommand, NULL);
  if (cli_execute_line(cli_get(), (char *) cCommand) != 0)
    cbgp_jni_throw_CBGPException(env, "could not execute command");
  (*env)->ReleaseStringUTFChars(env, jsCommand, cCommand);
}

// -----[ cbgp_jni_routes_list_function ]----------------------------
int cbgp_jni_routes_list_function(void * pItem, void * pContext)
{
  SRouteDumpCtx * pCtx= (SRouteDumpCtx *) pContext;
  jobject joRoute;

  if ((joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, *(SRoute **) pItem)) == NULL)
    return -1;

  return cbgp_jni_ArrayList_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ loadMRT ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    loadMRT
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_loadMRT
  (JNIEnv *env , jobject obj, jstring jsFileName)
{
  jobject joArrayList= NULL;
  SRoutes * pRoutes;
  const jbyte * pcFileName;
  SRouteDumpCtx sCtx;

  pcFileName= (*env)->GetStringUTFChars(env, jsFileName, NULL);
  if ((pRoutes= mrtd_load_routes(pcFileName, 0, NULL)) != NULL) {
    joArrayList= cbgp_jni_new_ArrayList(env);
    sCtx.jEnv= env;
    sCtx.joVector= joArrayList;
    if (routes_list_for_each(pRoutes, cbgp_jni_routes_list_function, &sCtx) != 0)
      joArrayList= NULL;
  }
  (*env)->ReleaseStringUTFChars(env, jsFileName, pcFileName);

  // TODO: clean whole list !!! (routes_list are only references...)
  routes_list_destroy(&pRoutes);

  fprintf(stderr, "Conversion to Java finished\n");

  return joArrayList;
}
