// ==================================================================
// @(#)jni_interface.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2003
// @lastdate 30/10/2004
// ==================================================================

#include <jni.h>
#include <string.h>
#include <assert.h>

#include <jni/cbgpJNI.h>

#include <net/network.h>
#include <net/protocol.h>
#include <net/prefix.h>
#include <net/igp.h>

#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/peer.h>
#include <bgp/filter.h>
#include <bgp/filter_t.h>

#include <sim/simulator.h>

#include <libgds/log.h>


#define FILTER_IN   "in"
#define FILTER_OUT  "out"

SFilter * pFilter;
SFilterMatcher * pMatcher;
SFilterAction * pAction;

net_addr_t ip_jstring_to_address(JNIEnv * env, jstring net_addr)
{
  const jbyte * cNetAddr;
  char * pcEndPtr;
  net_addr_t iNetAddr;

  cNetAddr = (*env)->GetStringUTFChars(env, net_addr, NULL);
  if (ip_string_to_address((char *)cNetAddr, &pcEndPtr, &iNetAddr))
    LOG_DEBUG("jni>can't convert string to ip address : %s\n", cNetAddr);
  (*env)->ReleaseStringUTFChars(env, net_addr, cNetAddr);

  return iNetAddr;
}

/*
 * Class:     cbgpJNI
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_init
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

/*
 * Class:     cbgpJNI
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_finalize
  (JNIEnv * env, jobject obj)
{
  simulator_done();
}

/*
 * Class:     be_ac_ucl_poms_repository_IGPWO_cbgp_jni
 * Method:    node_add
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeAdd
  (JNIEnv * env, jobject obj, jstring net_addr)
{
  SNetNode * pNode; 
  SNetwork * pNetwork = network_get();
  net_addr_t iNetAddr;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = node_create(iNetAddr);

  return network_add_node(pNetwork, pNode);
}

/*
 * Class:     be_ac_ucl_poms_repository_IGPWO_cbgp_jni
 * Method:    link_add
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkAdd
  (JNIEnv *env, jobject obj, jstring net_addr_src, jstring  net_addr_dst, jint Weight)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNodeSrc, * pNodeDst;
  net_addr_t iNetAddrSrc, iNetAddrDst;
 
  iNetAddrSrc = ip_jstring_to_address(env, net_addr_src);
  iNetAddrDst = ip_jstring_to_address(env, net_addr_dst);

  pNodeSrc = network_find_node(pNetwork, iNetAddrSrc);
  pNodeDst = network_find_node(pNetwork, iNetAddrDst);

  
  return node_add_link(pNodeSrc, pNodeDst, Weight, 1);
}

/*
 * Class:     cbgpJNI
 * Method:    nodeLinkWeight
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkWeight
  (JNIEnv * env, jobject obj, jstring net_addr1, jstring net_addr2, jint Weight)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode1, * pNode2;
  SNetLink * pLink1, * pLink2;
  net_addr_t iNetAddr1, iNetAddr2;

  iNetAddr1 = ip_jstring_to_address(env, net_addr1);
  iNetAddr2 = ip_jstring_to_address(env, net_addr2);

  pNode1 = network_find_node(pNetwork, iNetAddr1);
  pNode2 = network_find_node(pNetwork, iNetAddr2);
  
  pLink1 = node_find_link(pNode1, iNetAddr2);
  pLink2 = node_find_link(pNode2, iNetAddr1);
  
  link_set_igp_weight(pLink1, Weight);
  link_set_igp_weight(pLink2, Weight);
  return 0;
}

/*
 * Class:     cbgpJNI
 * Method:    nodeLinkUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkUp
  (JNIEnv * env, jobject obj, jstring net_addr1, jstring net_addr2)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode1, * pNode2;
  SNetLink * pLink1, * pLink2;
  net_addr_t iNetAddr1, iNetAddr2;

  iNetAddr1 = ip_jstring_to_address(env, net_addr1);
  iNetAddr2 = ip_jstring_to_address(env, net_addr2);

  pNode1 = network_find_node(pNetwork, iNetAddr1);
  pNode2 = network_find_node(pNetwork, iNetAddr2);

  //Throw an exception
/*  if (pNode1 == NULL || pNode2 == NULL)
    return 1*/
  
  pLink1 = node_find_link(pNode1, iNetAddr2);
  pLink2 = node_find_link(pNode2, iNetAddr1);

  //Throw an exception
  //if (pLink1 == NULL || pLink2 == NULL)
  //return 1

  link_set_state(pLink1, NET_LINK_FLAG_UP, 1);
  link_set_state(pLink2, NET_LINK_FLAG_UP, 1);
  return 0;
}
  
/*
 * Class:     cbgpJNI
 * Method:    nodeLinkDown
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkDown
  (JNIEnv * env, jobject obj, jstring net_addr1, jstring net_addr2)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode1, * pNode2;
  SNetLink * pLink1, * pLink2;
  net_addr_t iNetAddr1, iNetAddr2;

  iNetAddr1 = ip_jstring_to_address(env, net_addr1);
  iNetAddr2 = ip_jstring_to_address(env, net_addr2);

  pNode1 = network_find_node(pNetwork, iNetAddr1);
  pNode2 = network_find_node(pNetwork, iNetAddr2);
  //Throw an exception
/*  if (pNode1 == NULL || pNode2 == NULL)
    return 1*/
  
  pLink1 = node_find_link(pNode1, iNetAddr2);
  pLink2 = node_find_link(pNode2, iNetAddr1);

  //Throw an exception
  //if (pLink1 == NULL || pLink2 == NULL)
  //return 1

  link_set_state(pLink1, NET_LINK_FLAG_UP, 0);
  link_set_state(pLink2, NET_LINK_FLAG_UP, 0);
  return 0;
}

/*
 * Class:     cbgpJNI
 * Method:    nodeSpfPrefix
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeSpfPrefix
  (JNIEnv * env, jobject obj, jstring net_addr, jstring prefix)
{
  const jbyte * cPrefix;
  char * pcEndPtr;
  net_addr_t iNetAddr;
  SPrefix Prefix;
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;


  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);
  
  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", cPrefix);
  }
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  // Compute the SPF from the node towards all the nodes in the prefix
  return igp_compute_prefix(pNetwork, pNode, Prefix);
}

/*
 * Class:     be_ac_ucl_poms_repository_IGPWO_cbgp_jni
 * Method:    node_interface_add
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeInterfaceAdd
  (JNIEnv * env, jobject obj, jstring net_addr_id, jstring net_addr_int, jstring mask)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetInterface * pInterface; 

  const jbyte * cMask;
  char * pcEndPtr;
  net_addr_t iNetAddrId, iNetAddrInt;
  SPrefix * pMask = MALLOC(sizeof(SPrefix));
 
  iNetAddrId = ip_jstring_to_address(env, net_addr_id);
  pNode = network_find_node(pNetwork, iNetAddrId);

  iNetAddrInt = ip_jstring_to_address(env, net_addr_int);

  cMask = (*env)->GetStringUTFChars(env, mask, NULL);
  if (ip_string_to_prefix((char *)cMask, &pcEndPtr, pMask))
     LOG_DEBUG("jni>can't convert string to ip address : %s\n", cMask);
  (*env)->ReleaseStringUTFChars(env, mask, cMask);
  
  pInterface = node_interface_create();
  pInterface->tAddr = iNetAddrInt;
  pInterface->tMask = pMask;

  return node_interface_add(pNode, pInterface);
}


/*
 * Class:     cbgpJNI
 * Method:    nodeRouteAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeRouteAdd
  (JNIEnv * env, jobject obj, jstring net_addr, jstring prefix, jstring net_addr_next_hop, jint Weight)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;

  const jbyte * cPrefix;
  char * pcEndPtr;
  net_addr_t iNetAddr, iNetAddrNextHop;
  SPrefix Prefix;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);
  
  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix))
     LOG_DEBUG("jni>can't convert string to ip address : %s\n", cPrefix);
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  iNetAddrNextHop = ip_jstring_to_address(env, net_addr_next_hop);

  return node_rt_add_route(pNode, Prefix, iNetAddrNextHop, Weight, NET_ROUTE_STATIC);
}


/*
 * Class:     cbgpJNI
 * Method:    nodeShowLinks
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_nodeShowLinks
  (JNIEnv * env, jobject obj, jstring net_addr)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  net_addr_t iNetAddr;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);
  
  node_links_dump(stdout, pNode);
}


/*
 * Class:     cbgpJNI
 * Method:    nodeShowRT
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_jni_cbgp_cbgpJNI_nodeShowRT
  (JNIEnv * env, jobject obj, jstring net_addr, jstring prefix)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  net_addr_t iNetAddr;
  SPrefix Prefix;
  char * cRT;

  /*const jbyte * cPrefix;
  char * pcEndPtr;*/

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);

  Prefix.uMaskLen = 0;
  /*cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix))
     LOG_DEBUG("jni>can't convert string to ip address : %s\n", cPrefix);
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);*/

  cRT = node_rt_dump_string(pNode, Prefix);
  if (cRT == NULL) {
    cRT = MALLOC(1);
    strcpy(cRT, "");
  }
  return (*env)->NewStringUTF(env, cRT);
}

/*
 * Class:     be_ac_ucl_poms_repository_IGPWO_cbgp_jni
 * Method:    add_bgp_router
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterAdd
  (JNIEnv * env, jobject obj, jstring name, jstring net_addr, jint ASid)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SBGPRouter * pRouter;
  net_addr_t iNetAddr;

  const jbyte *  pcName;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);

  pRouter = as_create(ASid, pNode, 0);
  pcName = (*env)->GetStringUTFChars(env, name, NULL);
  as_add_name(pRouter, strdup((char *)pcName));
  assert(!node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter, 
			  (FNetNodeHandlerDestroy) as_destroy, 
			  as_handle_message));
  (*env)->ReleaseStringUTFChars(env, name, pcName);
  return 0;
}

/*
 * Class:     be_ac_ucl_poms_repository_IGPWO_cbgp_jni
 * Method:    add_bgp_router_network
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNetworkAdd
  (JNIEnv * env, jobject obj, jstring net_addr, jstring prefix)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode; 
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  net_addr_t iNetAddr;
  SPrefix Prefix;

  const jbyte *  cPrefix;
  char * pcEndPtr;

  iNetAddr = ip_jstring_to_address(env, net_addr);

  pNode = network_find_node(pNetwork, iNetAddr);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  cPrefix = (*env)->GetStringUTFChars(env, prefix, NULL);
  if (ip_string_to_prefix((char *)cPrefix, &pcEndPtr, &Prefix)) {
    LOG_DEBUG("jni> can't convert string to ip address : %s\n", prefix);
  }
  (*env)->ReleaseStringUTFChars(env, prefix, cPrefix);

  return as_add_network(pRouter, Prefix);
}

/*
 * Class:     be_ac_ucl_poms_repository_IGPWO_cbgp_jni
 * Method:    add_bgp_router_neighbor
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNeighborAdd
  (JNIEnv * env, jobject obj, jstring net_addr_router, jstring net_addr_neighbor, jint ASIdNeighbor)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  net_addr_t iNetAddrRouter, iNetAddrNeigh;

  iNetAddrRouter = ip_jstring_to_address(env, net_addr_router);
  pNode = network_find_node(pNetwork, iNetAddrRouter);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  iNetAddrNeigh = ip_jstring_to_address(env, net_addr_neighbor);
  
  return as_add_peer(pRouter, ASIdNeighbor, iNetAddrNeigh, 0);
}


/*
 * Class:     cbgpJNI
 * Method:    bgpRouterNeighborNextHopSelf
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNeighborNextHopSelf
  (JNIEnv * env, jobject obj, jstring net_addr_router, jstring net_addr_neighbor)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  SPeer * pPeer;
  net_addr_t iNetAddrRouter, iNetAddrNeigh;

  iNetAddrRouter = ip_jstring_to_address(env, net_addr_router);
  pNode = network_find_node(pNetwork, iNetAddrRouter);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  iNetAddrNeigh = ip_jstring_to_address(env, net_addr_neighbor);
 
  pPeer = as_find_peer(pRouter, iNetAddrNeigh);

  peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
}

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterNeighborUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNeighborUp
  (JNIEnv * env, jobject obj, jstring net_addr_router, jstring net_addr_neighbor)
{
  net_addr_t iNetAddrRouter, iNetAddrNeigh;
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  SPeer * pPeer;

  iNetAddrRouter = ip_jstring_to_address(env, net_addr_router);
  pNode = network_find_node(pNetwork, iNetAddrRouter);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  iNetAddrNeigh = ip_jstring_to_address(env, net_addr_neighbor);
  
  pPeer = as_find_peer(pRouter, iNetAddrNeigh);

  return peer_open_session(pPeer);
}


/*
 * Class:     cbgpJNI
 * Method:    bgpRouterRescan
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterRescan
  (JNIEnv * env, jobject obj, jstring net_addr)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  net_addr_t iNetAddr;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;
 
  return bgp_router_scan_rib(pRouter);
}

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterShowRib
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterShowRib
  (JNIEnv * env, jobject obj, jstring net_addr)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  net_addr_t iNetAddr;
  char * cRIB;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  cRIB = bgp_router_dump_rib_string(pRouter);
  if (cRIB == NULL) {
    cRIB = MALLOC(1);
    strcpy(cRIB, "");
  }
  return (*env)->NewStringUTF(env, cRIB);
}

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterShowRibIn
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterShowRibIn
  (JNIEnv * env, jobject obj, jstring net_addr)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  net_addr_t iNetAddr;
  SPrefix Prefix;

  Prefix.uMaskLen = 0;

  iNetAddr = ip_jstring_to_address(env, net_addr);
  pNode = network_find_node(pNetwork, iNetAddr);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  bgp_router_dump_ribin(stdout, pRouter, NULL, Prefix);
}

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterInit
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterInit
  (JNIEnv * env, jobject obj, jstring net_addr_router, jstring net_addr_neighbor, 
   jstring type)
{
  SNetwork * pNetwork = network_get();
  SNetNode * pNode;
  SNetProtocol * pProtocol;
  SBGPRouter * pRouter;
  SPeer * pPeer;
  net_addr_t iNetAddrRouter, iNetAddrNeigh;

  int ret =0;
  const jbyte * cType;

  iNetAddrRouter = ip_jstring_to_address(env, net_addr_router);
  pNode = network_find_node(pNetwork, iNetAddrRouter);
  pProtocol = protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP);
  pRouter = pProtocol->pHandler;

  iNetAddrNeigh = ip_jstring_to_address(env, net_addr_neighbor);
  
  pPeer = as_find_peer(pRouter, iNetAddrNeigh);

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

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterMatchPrefix
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterMatchPrefix
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
  /*filter_matcher_dump(stdout, pMatcher);
  printf("\n");*/
  return 0;
}

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterAction
 * Signature: (ILjava/lang/String;)I
 *
 */
#define ACTION_PERMIT	    0x01
#define ACTION_DENY	    0x02
#define ACTION_LOCPREF	    0x03
#define ACTION_ADD_COMM	    0x04
#define ACTION_PATH_PREPEND 0x05
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterAction
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
  /*filter_action_dump(stdout, pAction);
  fprintf(stdout, "\n");*/
  return 0;
}

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterFinalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterFinalize
  (JNIEnv *env , jobject obj)
{
  pFilter = NULL;
  pMatcher = NULL;
  pAction = NULL;
}


/*
 * Class:     cbgpJNI
 * Method:    simRun
 * Signature: ()I
 */
JNIEXPORT int JNICALL Java_jni_cbgp_cbgpJNI_simRun
  (JNIEnv * env, jobject obj)
{
  if (simulator_run())
    return 1; 
  return 0;
}

/*
 * Class:     cbgpJNI
 * Method:    simPrint
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_simPrint
  (JNIEnv * env, jobject obj, jstring line)
{
  const jbyte * cLine;

  cLine = (*env)->GetStringUTFChars(env, line, NULL);
  fprintf(stdout, "%s", cLine);
  (*env)->ReleaseStringUTFChars(env, line, cLine);
}


