// ==================================================================
// @(#)auto-config.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/11/2005
// @lastdate 11/09/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <bgp/auto-config.h>

#include <assert.h>

#include <libgds/log.h>

#include <bgp/as.h>
#include <bgp/peer.h>
#include <net/network.h>
#include <net/node.h>

#define AUTO_CONFIG_LINK_WEIGHT 1

// -----[ bgp_auto_config_session ]----------------------------------
/**
 * This function creates a BGP session between the given router and
 * a remote router.
 *
 * The function first checks if the remote address corresponds to an
 * existing node. If not, the node is created. Then, the function
 * checks if a direct link exists between the router and the remote
 * node. If not, an unidirectional link is created. The IGP weight
 * associated with the link is AUTO_CONFIG_LINK_WEIGHT. In addition,
 * the link will not be advertised in the IGP. The reason for this is
 * that we do not want that further recomputation of the domain's IGP
 * routes take this link into account.
 *
 * The next operation for the function is now to check if the router
 * has a route towards the remote node. If not the function will add
 * in the router a static route towards the remote node. In addition,
 * if there was no route, the 'next-hop-self' option will be used on
 * the new peering session since the remote node will not be reachable
 * from other routers in the domain.
 *
 * Finaly, the function will check if the remote node supports the BGP
 * protocol. If not, support for the BGP protocol will not be added,
 * but the 'virtual' option will be used on the new peering
 * session. The new peering session can now be added. If the remote
 * node supports BGP, the BGP session must be added on both
 * sides. Otherwise, it must only be added on the local router's side.
 */
int bgp_auto_config_session(SBGPRouter * pRouter,
			    net_addr_t tRemoteAddr,
			    uint16_t uRemoteAS,
			    SBGPPeer ** ppPeer)
{
  SNetNode * pNode;
  SNetLink * pLink;
  SPrefix sPrefix;
  int iUseNextHopSelf= 0;
  SBGPPeer * pPeer;

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
    log_printf(pLogDebug, "AUTO-CONFIG ");
    bgp_router_dump_id(pLogDebug, pRouter);
    log_printf(pLogDebug, " NEIGHBOR AS%d:", uRemoteAS);
    ip_address_dump(pLogDebug, tRemoteAddr);
    log_printf(pLogDebug, "\n");
  }

  // (1). If node does not exist, create it.
  LOG_DEBUG(LOG_LEVEL_DEBUG, "PHASE (1) CHECK NODE EXISTENCE\n");
  pNode= network_find_node(tRemoteAddr);
  if (pNode == NULL) {
    pNode= node_create(tRemoteAddr);
    network_add_node(pNode);
  }
  
  // (2). If there is no direct link to the node, create it. The
  // reason for this choice is that we create the neighbors based on a
  // RIB dump and the neighbors are supposed to be connected over
  // single-hop eBGP sessions.
  LOG_DEBUG(LOG_LEVEL_DEBUG, "PHASE (2) CHECK LINK EXISTENCE\n");
  pLink= node_find_link_to_router(pRouter->pNode, tRemoteAddr);
  if (pLink == NULL) {
    
    // Create the link in one direction only. IGP weight is set
    // to AUTO_CONFIG_LINK_WEIGHT. The IGP_ADV flag is also removed
    // from the new link.
    assert(node_add_link_to_router(pRouter->pNode, pNode,
				   AUTO_CONFIG_LINK_WEIGHT, 0) >= 0);
    pLink= node_find_link_to_router(pRouter->pNode, tRemoteAddr);
    assert(pLink != NULL);
    link_set_state(pLink, NET_LINK_FLAG_IGP_ADV, 0);
  }    

  // (3). Check if there is a route towards the remote node.
  LOG_DEBUG(LOG_LEVEL_DEBUG, "PHASE (3) CHECK ROUTE EXISTENCE\n");
  if (node_rt_lookup(pRouter->pNode, tRemoteAddr) == NULL) {
    // We should also add a route towards this node. Let's
    // assume that 'next-hop-self' will be used for this session
    // and add a static route towards the next-hop. The link is
    // therefore not advertised within the IGP domain.
    sPrefix.tNetwork= tRemoteAddr;
    sPrefix.uMaskLen= 32;
    assert(node_rt_add_route_link(pRouter->pNode, sPrefix, pLink,
				  tRemoteAddr, AUTO_CONFIG_LINK_WEIGHT,
				  NET_ROUTE_STATIC) == 0);
    iUseNextHopSelf= 1;
  }
  
  // Add a new BGP session.
  LOG_DEBUG(LOG_LEVEL_DEBUG, "PHASE (4) ADD BGP SESSION ");
  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
     ip_address_dump(pLogDebug, pRouter->pNode->tAddr);
    log_printf(pLogDebug, " --> ");
    ip_address_dump(pLogDebug, tRemoteAddr);
    log_printf(pLogDebug, "\n");
  }
  pPeer= bgp_router_add_peer(pRouter, uRemoteAS,
			     tRemoteAddr, 0);
  if (pPeer == NULL) {
    LOG_ERR(LOG_LEVEL_FATAL, "ERROR: could not create peer\n");
    abort();
  }
  
  // If peer does not support BGP, create it virtual. Otherwise, also
  // create the session in the remote BGP router.
  if (protocols_get(pNode->pProtocols, NET_PROTOCOL_BGP) == NULL)
    bgp_peer_flag_set(pPeer, PEER_FLAG_VIRTUAL, 1);
  else {
    // TODO: we should create the BGP session in the reverse
    // direction...
    LOG_ERR(LOG_LEVEL_FATAL, "ERROR: Code not implemented\n");
    abort();
  }
  
  // Set the next-hop-self flag (this is required since the
  // next-hop is not advertised in the IGP).
  if (iUseNextHopSelf)
    bgp_peer_flag_set(pPeer, PEER_FLAG_NEXT_HOP_SELF, 1);
  
  // Open the BGP session.
  assert(bgp_peer_open_session(pPeer) == 0);

  LOG_DEBUG(LOG_LEVEL_DEBUG, "DONE :-)\n");

  *ppPeer= pPeer;

  return 0;
}
