// ==================================================================
// @(#)route.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/11/2002
// @lastdate 22/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/array.h>
#include <libgds/memory.h>

#include <bgp/attr.h>
#include <bgp/comm.h>
#include <bgp/comm_hash.h>
#include <bgp/ecomm.h>
#include <bgp/path_hash.h>
#include <bgp/qos.h>
#include <bgp/route.h>
#include <util/str_format.h>

// -----[ Forward prototypes declaration ]---------------------------
/* Note: functions starting with underscore (_) are intended to be
 * used inside this file only (private). These functions should be
 * static and should not appear in the .h file.
 */
SRoute * route_create2(SPrefix sPrefix, SBGPPeer * pPeer,
		       SBGPAttr * pAttr);

// ----- route_create -----------------------------------------------
/**
 * Create a new BGP route towards the given prefix. The 'peer' field
 * mentions the peer who announced the route to the local router. This
 * field can be NULL if the route was originated locally or if the
 * peer is unknown. The 'next-hop' field is the BGP next-hop of the
 * route. Finally, the 'origin-type' field tells how the route was
 * originated. This is usually set to IGP.
 */
SRoute * route_create(SPrefix sPrefix, SBGPPeer * pPeer,
		      net_addr_t tNextHop,
		      bgp_origin_t tOrigin)
{
  return route_create2(sPrefix, pPeer,
		       bgp_attr_create(tNextHop, tOrigin,
				       ROUTE_PREF_DEFAULT,
				       ROUTE_MED_DEFAULT));
}

// -----[ route_create2 ]--------------------------------------------
/**
 *
 */
SRoute * route_create2(SPrefix sPrefix, SBGPPeer * pPeer,
		       SBGPAttr * pAttr)
{
  SRoute * pRoute= (SRoute *) MALLOC(sizeof(SRoute));
  pRoute->sPrefix= sPrefix;
  pRoute->pPeer= pPeer;
  pRoute->pAttr= pAttr;
  pRoute->uFlags= 0;

  /* QoS fields */
#ifdef BGP_QOS
  qos_route_init_delay(pRoute);
  qos_route_init_bandwidth(pRoute);
  pRoute->pAggrRoutes= NULL;
  pRoute->pAggrRoute= NULL;
  pRoute->pEBGPRoute= NULL;
#endif

#ifdef __ROUTER_LIST_ENABLE__
  pRoute->pRouterList= NULL;
#endif

#ifdef __EXPERIMENTAL__
  pRoute->pOriginRouter= NULL;
#endif

#ifdef __BGP_ROUTE_INFO_DP__
    pRoute->tRank= 0;
#endif

  return pRoute;
}

// ----- route_destroy ----------------------------------------------
/**
 * Destroy the given route.
 */
void route_destroy(SRoute ** ppRoute)
{
  if (*ppRoute != NULL) {

    /* BGP QoS */
#ifdef BGP_QOS
    ptr_array_destroy(&(*ppRoute)->pAggrRoutes);
#endif

#ifdef __ROUTER_LIST_ENABLE__
    route_router_list_clear(*ppRoute);
#endif

    bgp_attr_destroy(&(*ppRoute)->pAttr);

    FREE(*ppRoute);
    *ppRoute= NULL;
  }

}

// ----- route_flag_set ---------------------------------------------
/**
 *
 */
inline void route_flag_set(SRoute * pRoute, uint16_t uFlag,
			   int iState)
{
  if (iState)
    pRoute->uFlags|= uFlag;
  else
    pRoute->uFlags&= ~uFlag;
}

// ----- route_flag_get ---------------------------------------------
/**
 *
 */
inline int route_flag_get(SRoute * pRoute, uint16_t uFlag)
{
  return pRoute->uFlags & uFlag;
}

// ----- route_peer_set ---------------------------------------------
/**
 *
 */
inline void route_peer_set(SRoute * pRoute, SBGPPeer * pPeer)
{
  pRoute->pPeer= pPeer;
}

// ----- route_peer_get ---------------------------------------------
/**
 *
 */
inline SBGPPeer * route_peer_get(SRoute * pRoute)
{
  assert(pRoute->pPeer != NULL);
  return pRoute->pPeer;
}

#ifdef __EXPERIMENTAL__
// ----- route_set_origin_router ------------------------------------
inline void route_set_origin_router(SRoute * pRoute,
				    SBGPRouter * pRouter)
{
  pRoute->pOriginRouter= pRouter;
}
#endif

/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE NEXT-HOP
//
/////////////////////////////////////////////////////////////////////

// ----- route_set_nexthop ------------------------------------------
/**
 * Set the route's BGP next-hop.
 */
inline void route_set_nexthop(SRoute * pRoute,
			      net_addr_t tNextHop)
{
  bgp_attr_set_nexthop(&pRoute->pAttr, tNextHop);
}

// ----- route_get_nexthop ------------------------------------------
/**
 * Return the route's BGP next-hop.
 */
inline net_addr_t route_get_nexthop(SRoute * pRoute)
{
  return pRoute->pAttr->tNextHop;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGIN
//
/////////////////////////////////////////////////////////////////////

// ----- route_set_origin -------------------------------------------
/**
 * Set the route's origin.
 */
inline void route_set_origin(SRoute * pRoute, bgp_origin_t tOrigin)
{
  bgp_attr_set_origin(&pRoute->pAttr, tOrigin);
}

// ----- route_get_origin -------------------------------------------
/**
 * Return the route's origin.
 */
inline bgp_origin_t route_get_origin(SRoute * pRoute)
{
  return pRoute->pAttr->tOrigin;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE AS-PATH
//
/////////////////////////////////////////////////////////////////////

// -----[ route_set_path ]-------------------------------------------
/**
 * Set the route's AS-Path and adds a global reference if required.
 *
 * Note: the given AS-Path is not copied, but it will be referenced
 * in the global path repository if used or destroyed if not.
 */
inline void route_set_path(SRoute * pRoute, SBGPPath * pPath)
{
  bgp_attr_set_path(&pRoute->pAttr, pPath);
}

// -----[ route_get_path ]-------------------------------------------
/**
 * Return the route's AS-Path.
 */
inline SBGPPath * route_get_path(SRoute * pRoute)
{
  return pRoute->pAttr->pASPathRef;
}

// -----[ route_path_prepend ]---------------------------------------
/**
 * Prepend multiple times an AS-Number at the beginning of this
 * AS-Path. This is used when a route is propagated over an eBGP
 * session, for instance.
 *
 * The function first creates a copy of the current path. Then, it
 * removes the current AS-Path from the global path
 * repository. Finally, it appends the given AS-number to the AS-Path
 * copy. If the prepending could be done without error, the resulting
 * path is set as the route's AS-Path.
 *
 * Return value:
 *   0  in case of success
 *  -1  in case of failure (AS-Path would become too long)
 */
inline int route_path_prepend(SRoute * pRoute, uint16_t uAS,
			      uint8_t uAmount)
{
  return bgp_attr_path_prepend(&pRoute->pAttr, uAS, uAmount);
}

// -----[ route_path_rem_private ]-----------------------------------
inline int route_path_rem_private(SRoute * pRoute)
{
  return bgp_attr_path_rem_private(&pRoute->pAttr);
}

// ----- route_path_contains ----------------------------------------
/**
 * Test if the route's AS-PAth contains the given AS-number.
 *
 * Returns 1 if true, 0 otherwise.
 */
inline int route_path_contains(SRoute * pRoute, uint16_t uAS)
{
  return (pRoute->pAttr->pASPathRef != NULL) &&
    (path_contains(pRoute->pAttr->pASPathRef, uAS) != 0);
}

// ----- route_path_length ------------------------------------------
/**
 * Return the length of the route's AS-Path.
 */
inline int route_path_length(SRoute * pRoute)
{
  if (pRoute->pAttr->pASPathRef != NULL)
    return path_length(pRoute->pAttr->pASPathRef);
  else
    return 0;
}

// ----- route_path_last_as -----------------------------------------
/**
 * Return the last AS-number in the AS-Path. This function is
 * currently used in dp_rules.c in order to implement the MED-based
 * rule.
 *
 * Note: this function is EXPERIMENTAL and should be updated. It does
 * not work if the last segment in the AS-Path is of type AS-SET since
 * there is no ordering of the AS-numbers in this case.
 */
inline int route_path_last_as(SRoute * pRoute)
{
  if (pRoute->pAttr->pASPathRef != NULL)
    return path_last_as(pRoute->pAttr->pASPathRef);
  else
    return -1;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// ----- route_set_comm ---------------------------------------------
/**
 * Set the route's Communities attribute and adds a global reference
 * if required.
 *
 * Note: the given Communities is not copied, but it will be
 * referenced in the global Communities repository if used or
 * destroyed if not.
 */
inline void route_set_comm(SRoute * pRoute, SCommunities * pCommunities)
{
  bgp_attr_set_comm(&pRoute->pAttr, pCommunities);
}

// ----- route_comm_append ------------------------------------------
/**
 * Add a new community value to the Communities attribute.
 *
 * Return value:
 *    0  in case of success
 *   -1  in case of failure (Communities would become too long)
 */
inline int route_comm_append(SRoute * pRoute, comm_t tCommunity)
{
  return bgp_attr_comm_append(&pRoute->pAttr, tCommunity);
}

// ----- route_comm_strip -------------------------------------------
/**
 * Remove all the community values from the Communities attribute.
 */
inline void route_comm_strip(SRoute * pRoute)
{
  bgp_attr_comm_strip(&pRoute->pAttr);
}

// ----- route_comm_remove ------------------------------------------
/**
 * Remove a single community value from the Communities attribute.
 */
inline void route_comm_remove(SRoute * pRoute, comm_t tCommunity)
{
  bgp_attr_comm_remove(&pRoute->pAttr, tCommunity);
}

// ----- route_comm_contains ----------------------------------------
/**
 * Test if the Communities attribute contains the given community
 * value.
 */
inline int route_comm_contains(SRoute * pRoute, comm_t tCommunity)
{
  return (pRoute->pAttr->pCommunities != NULL) &&
    (comm_contains(pRoute->pAttr->pCommunities, tCommunity) != 0);
}

/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE EXTENDED-COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// ----- route_ecomm_append -----------------------------------------
/**
 *
 */
inline int route_ecomm_append(SRoute * pRoute, SECommunity * pComm)
{
  return bgp_attr_ecomm_append(&pRoute->pAttr, pComm);
}

// ----- route_ecomm_strip_non_transitive ---------------------------
/**
 *
 */
inline void route_ecomm_strip_non_transitive(SRoute * pRoute)
{
  ecomm_strip_non_transitive(&pRoute->pAttr->pECommunities);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE LOCAL-PREF
//
/////////////////////////////////////////////////////////////////////

// ----- route_localpref_set ----------------------------------------
/**
 *
 */
inline void route_localpref_set(SRoute * pRoute, uint32_t uPref)
{
  pRoute->pAttr->uLocalPref= uPref;
}

// ----- route_localpref_get ----------------------------------------
/**
 *
 */
inline uint32_t route_localpref_get(SRoute * pRoute)
{
  return pRoute->pAttr->uLocalPref;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE MULTI-EXIT-DISCRIMINATOR (MED)
//
/////////////////////////////////////////////////////////////////////

// ----- route_med_clear --------------------------------------------
/**
 *
 */
inline void route_med_clear(SRoute * pRoute)
{
  pRoute->pAttr->uMED= ROUTE_MED_MISSING;
}

// ----- route_med_set ----------------------------------------------
/**
 *
 */
inline void route_med_set(SRoute * pRoute, uint32_t uMED)
{
  pRoute->pAttr->uMED= uMED;
}

// ----- route_med_get ----------------------------------------------
/**
 *
 */
inline uint32_t route_med_get(SRoute * pRoute)
{
  return pRoute->pAttr->uMED;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGINATOR
//
/////////////////////////////////////////////////////////////////////

// ----- route_originator_set ---------------------------------------
/**
 *
 */
inline void route_originator_set(SRoute * pRoute, net_addr_t tOriginator)
{
  assert(pRoute->pAttr->pOriginator == NULL);
  pRoute->pAttr->pOriginator= (net_addr_t *) MALLOC(sizeof(net_addr_t));
  memcpy(pRoute->pAttr->pOriginator, &tOriginator, sizeof(net_addr_t));
}

// ----- route_originator_get ---------------------------------------
/**
 * Return the route's Originator-ID (if set).
 *
 * Return values:
 *    0 if Originator-ID exists
 *   -1 otherwise
 */
inline int route_originator_get(SRoute * pRoute, net_addr_t * pOriginator)
{
  if (pRoute->pAttr->pOriginator != NULL) {
    if (pOriginator != NULL)
      memcpy(pOriginator, pRoute->pAttr->pOriginator, sizeof(net_addr_t));
    return 0;
  } else
    return -1;
}

// ----- route_originator_clear -------------------------------------
/**
 *
 */
inline void route_originator_clear(SRoute * pRoute)
{
  bgp_attr_originator_destroy(pRoute->pAttr);
}

// ----- route_originator_equals ------------------------------------
/**
 * Compare the Originator-ID of two routes.
 *
 * Return value:
 *   1  if both originator-IDs are equal
 *   0  otherwise
 */
inline int route_originator_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  return originator_equals(pRoute1->pAttr->pOriginator,
			   pRoute2->pAttr->pOriginator);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE CLUSTER-ID-LIST
//
/////////////////////////////////////////////////////////////////////

// ----- route_cluster_list_set -------------------------------------
/**
 *
 */
inline void route_cluster_list_set(SRoute * pRoute)
{
  assert(pRoute->pAttr->pClusterList == NULL);
  pRoute->pAttr->pClusterList= cluster_list_create();
}

// ----- route_cluster_list_append ----------------------------------
/**
 *
 */
inline void route_cluster_list_append(SRoute * pRoute,
				      cluster_id_t tClusterID)
{
  if (pRoute->pAttr->pClusterList == NULL)
    route_cluster_list_set(pRoute);
  cluster_list_append(pRoute->pAttr->pClusterList, tClusterID);
}

// ----- route_cluster_list_clear -----------------------------------
/**
 *
 */
void route_cluster_list_clear(SRoute * pRoute)
{
  bgp_attr_cluster_list_destroy(pRoute->pAttr);
}

// ----- route_cluster_list_equals ----------------------------------
/**
 * Compare the Cluster-ID-List of two routes.
 *
 * Return value:
 *   1  if both Cluster-ID-Lists are equal
 *   0  otherwise
 */
inline int route_cluster_list_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  return cluster_list_equals(pRoute1->pAttr->pClusterList,
			     pRoute2->pAttr->pClusterList);
}

// ----- route_cluster_list_contains --------------------------------
/**
 *
 */
inline int route_cluster_list_contains(SRoute * pRoute,
				       cluster_id_t tClusterID)
{
  if (pRoute->pAttr->pClusterList != NULL)
    return cluster_list_contains(pRoute->pAttr->pClusterList, tClusterID);
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ROUTER-LIST (FOR DEBUGGING PURPOSES)
//
/////////////////////////////////////////////////////////////////////

#ifdef __ROUTER_LIST_ENABLE__
// ----- route_router_list_append -----------------------------------
inline void route_router_list_append(SRoute * pRoute, net_addr_t tAddr)
{
  if (pRoute->pRouterList == NULL)
    pRoute->pRouterList= cluster_list_create();
  cluster_list_append(pRoute->pRouterList, tAddr);
}

// ----- route_router_list_clear ------------------------------------
inline void route_router_list_clear(SRoute * pRoute)
{
  cluster_list_destroy(pRoute->pRouterList);
}

// ----- route_router_list_copy -------------------------------------
inline SClusterList * route_router_list_copy(SRoute * pRoute)
{
  if (pRoute->pRouterList != NULL)
    return cluster_list_copy(pRoute->pRouterList);
  return NULL;
}
#endif /* __ROUTER_LIST_ENABLE__ */


/////////////////////////////////////////////////////////////////////
//
// ROUTE COPY & COMPARE
//
/////////////////////////////////////////////////////////////////////

// ----- route_equals -----------------------------------------------
/**
 *
 */
int route_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  if (pRoute1 == pRoute2) {
    return 1;
  }

  /* BGP QoS */
#ifdef BGP_QOS
  if (!qos_route_delay_equals(pRoute1, pRoute2) ||
      !qos_route_bandwidth_equals(pRoute1, pRoute2)) {
    return 0;
  }
#endif

  // Compare destination prefixes
  if (ip_prefix_cmp(&pRoute1->sPrefix, &pRoute2->sPrefix))
    return 0;

  if (bgp_attr_cmp(pRoute1->pAttr, pRoute2->pAttr)) {
    return 1;
  }
  return 0;
}

// ----- route_copy -------------------------------------------------
/**
 *
 */
SRoute * route_copy(SRoute * pRoute)
{
  SRoute * pNewRoute= route_create2(pRoute->sPrefix,
				    pRoute->pPeer,
				    bgp_attr_copy(pRoute->pAttr));

  /* Route info */
  pNewRoute->uFlags= pRoute->uFlags;

  /* QoS attributes (experimental) */
#ifdef BGP_QOS
  pNewRoute->tDelay= pRoute->tDelay;
  pNewRoute->tBandwidth= pRoute->tBandwidth;
  // Copy also list of aggregatable routes ??
  // Normalement non: alloué par le decision process et références
  // dans Adj-RIB-Outs. A vérifier !!!
  if (pRoute->pAggrRoute != NULL) {
    pNewRoute->pAggrRoutes=
      (SPtrArray *) _array_copy((SArray *) pRoute->pAggrRoutes);
    pNewRoute->pAggrRoute= route_copy(pRoute->pAggrRoute);
  }
#endif

#ifdef __ROUTER_LIST_ENABLE__
  pNewRoute->pRouterList= route_router_list_copy(pRoute);
#endif

#ifdef __BGP_ROUTE_DP_INFO__
  pNewRoute->tRank= pRoute->tRank;
#endif

  return pNewRoute;
}


/////////////////////////////////////////////////////////////////////
//
// ROUTE DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ route_str2format ]-----------------------------------------
int route_str2format(const char * pcFormat, uint8_t * puFormat)
{
  if (!strcmp(pcFormat, "cisco") || !strcmp(pcFormat, "default")) {
    *puFormat= BGP_ROUTES_OUTPUT_CISCO;
    return 0;
  } else if (!strcmp(pcFormat, "mrt") || !strcmp(pcFormat, "mrt-ascii")) {
    *puFormat= BGP_ROUTES_OUTPUT_MRT_ASCII;
    return 0;
  } else if (!strcmp(pcFormat, "custom")) {
    *puFormat= BGP_ROUTES_OUTPUT_CUSTOM;
    return 0;
  }
  return -1;
}

// ----- route_dump -------------------------------------------------
/**
 * Dump a BGP route.
 */
void route_dump(SLogStream * pStream, SRoute * pRoute)
{
  switch (BGP_OPTIONS_SHOW_MODE) {
  case BGP_ROUTES_OUTPUT_MRT_ASCII:
    route_dump_mrt(pStream, pRoute);
    break;
  case BGP_ROUTES_OUTPUT_CUSTOM:
    route_dump_custom(pStream, pRoute, BGP_OPTIONS_SHOW_FORMAT);
    break;
  case BGP_ROUTES_OUTPUT_CISCO:
  default:
    route_dump_cisco(pStream, pRoute);
  }
}

// ----- route_dump_cisco -------------------------------------------
/**
 * CISCO-like dump of routes.
 */
void route_dump_cisco(SLogStream * pStream, SRoute * pRoute)
{
  if (pRoute != NULL) {

    // Status code:
    // '*' - valid (feasible)
    // 'i' - internal (learned through 'add network')
    if (route_flag_get(pRoute, ROUTE_FLAG_INTERNAL))
      log_printf(pStream, "i");
    else if (route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))
      log_printf(pStream, "*");
    else
      log_printf(pStream, " ");

    // Best ?
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST))
      log_printf(pStream, ">");
    else
      log_printf(pStream, " ");

    log_printf(pStream, " ");

    // Prefix
    ip_prefix_dump(pStream, pRoute->sPrefix);

    // Next-Hop
    log_printf(pStream, "\t");
    ip_address_dump(pStream, pRoute->pAttr->tNextHop);

    // Local-Pref & Multi-Exit-Distriminator
    log_printf(pStream, "\t%u\t%u\t", pRoute->pAttr->uLocalPref,
	       pRoute->pAttr->uMED);

    // AS-Path
    if (pRoute->pAttr->pASPathRef != NULL)
      path_dump(pStream, pRoute->pAttr->pASPathRef, 1);
    else
      log_printf(pStream, "null");

    // Origin
    log_printf(pStream, "\t");
    switch (pRoute->pAttr->tOrigin) {
    case ROUTE_ORIGIN_IGP: log_printf(pStream, "i"); break;
    case ROUTE_ORIGIN_EGP: log_printf(pStream, "e"); break;
    case ROUTE_ORIGIN_INCOMPLETE: log_printf(pStream, "?"); break;
    }

#ifdef __ROUTER_LIST_ENABLE__
    // Router list
    log_printf(pStream, "\t[");
    if (pRoute->pRouterList != NULL)
      cluster_list_dump(pStream, pRoute->pRouterList);
    log_printf(pStream, "]");
#endif

  } else
    log_printf(pStream, "(null)");
}

// ----- route_dump_mrt --------------------------------------------
/**
 * Dump a route in MRTD format. The output has thus the following
 * format:
 *
 *   <best>|<peer-IP>|<peer-AS>|<prefix>|<AS-path>|<origin>|
 *   <next-hop>|<local-pref>|<med>|<comm>|<ext-comm>|
 *
 * where <best> is "B" if the route is marked as best and "?"
 * otherwise. Note that the <ext-comm> and <delay> fields are not
 * standard MRTD fields.
 *
 * Additional fields are dumped which are not standard:
 *
 *   <delay>|<delay-mean>|<delay-weight>|
 *   <bandwidth>|<bandwidth-mean>|<bandwidth-weight>
 *
 */
void route_dump_mrt(SLogStream * pStream, SRoute * pRoute)
{
  if (pRoute == NULL) {
    log_printf(pStream, "null");
  } else {

    /* BEST flag: in MRT's route_btoa, table dumps only contain best
       routes, so that a "B" is written. In our case, when dumping an
       Adj-RIB-In, non-best routes might exist. For these routes, a
       "_" is written. */
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {
      log_printf(pStream, "B|");
    } else {
      log_printf(pStream, "_|");
    }

    /* Peer-IP: if the route is local, the keyword LOCAL is written in
       place of the peer's IP address and AS number. */
    if (pRoute->pPeer != NULL) {
      ip_address_dump(pStream, pRoute->pPeer->tAddr);
      // Peer-AS
      log_printf(pStream, "|%u|", pRoute->pPeer->uRemoteAS);
    } else {
      log_printf(pStream, "LOCAL|LOCAL|");
    }

    /* Prefix */
    ip_prefix_dump(pStream, pRoute->sPrefix);
    log_printf(pStream, "|");

    /* AS-Path */
    path_dump(pStream, pRoute->pAttr->pASPathRef, 1);
    log_printf(pStream, "|");

    /* Origin */
    switch (pRoute->pAttr->tOrigin) {
    case ROUTE_ORIGIN_IGP: log_printf(pStream, "IGP"); break;
    case ROUTE_ORIGIN_EGP: log_printf(pStream, "EGP"); break;
    case ROUTE_ORIGIN_INCOMPLETE: log_printf(pStream, "INCOMPLETE"); break;
    default:
      log_printf(pStream, "???");
    }
    log_printf(pStream, "|");

    /* Next-Hop */
    ip_address_dump(pStream, pRoute->pAttr->tNextHop);

    /* Local-Pref */
    log_printf(pStream, "|%u|", pRoute->pAttr->uLocalPref);

    /* Multi-Exit-Discriminator */
    if (pRoute->pAttr->uMED != ROUTE_MED_MISSING)
      log_printf(pStream, "%u|", pRoute->pAttr->uMED);
    else
      log_printf(pStream, "|");

    /* Communities: written as numeric values */
    if (pRoute->pAttr->pCommunities != NULL)
      comm_dump(pStream, pRoute->pAttr->pCommunities, COMM_DUMP_RAW);
    log_printf(pStream, "|");

    /* Extended-Communities: written as numeric values */
    if (pRoute->pAttr->pECommunities != NULL)
      ecomm_dump(pStream, pRoute->pAttr->pECommunities, ECOMM_DUMP_RAW);
    log_printf(pStream, "|");

    // Route-reflectors: Originator-ID
    if (pRoute->pAttr->pOriginator != NULL)
      ip_address_dump(pStream, *pRoute->pAttr->pOriginator);
    log_printf(pStream, "|");

    // Route-reflectors: Cluster-ID-List
    if (pRoute->pAttr->pClusterList != NULL)
      cluster_list_dump(pStream, pRoute->pAttr->pClusterList);
    log_printf(pStream, "|");
    
#ifdef __ROUTER_LIST_ENABLE__
    // Router list
    if (pRoute->pRouterList != NULL)
      cluster_list_dump(pStream, pRoute->pRouterList);
    log_printf(pStream, "|");
#endif

    /*
    // QoS Delay
    fprintf(pStream, "%u|%u|%u|", pRoute->tDelay.tDelay,
	    pRoute->tDelay.tMean, pRoute->tDelay.uWeight);
    // QoS Bandwidth
    fprintf(pStream, "%u|%u|%u|", pRoute->tBandwidth.uBandwidth,
	    pRoute->tBandwidth.uMean, pRoute->tBandwidth.uWeight);
    */

  }
}

// ----- route_dump_custom ------------------------------------------
/**
 * Dump a BGP route to the given stream. The route fields are written
 * to the stream according to a customizable format specifier string.
 *
 * Specification  | Field
 * ---------------+------------------------
 * %i             | Peer-IP
 * %a             | Peer-AS
 * %p             | Prefix
 * %l             | Local-Preference
 * %P             | AS-Path
 * %m             | MED
 * %o             | Origin
 * %n             | Next-Hop
 * %c             | Communities
 * %e             | Extended-Communities
 * %O             | Originator-ID
 * %C             | Cluster-ID-List
 * %A             | origin AS
 */
static int _route_dump_custom(SLogStream * pStream, void * pContext,
			      char cFormat)
{
  SRoute * pRoute= (SRoute *) pContext;
  int iASN;
  switch (cFormat) {
  case 'i':
    if (pRoute->pPeer != NULL)
      ip_address_dump(pStream, pRoute->pPeer->tAddr);
    else
      log_printf(pStream, "LOCAL");
    break;
  case 'a':
    if (pRoute->pPeer != NULL)
      log_printf(pStream, "%u", pRoute->pPeer->uRemoteAS);
    else
      log_printf(pStream, "LOCAL");
    break;
  case 'p':
    ip_prefix_dump(pStream, pRoute->sPrefix);
    break;
  case 'l':
    log_printf(pStream, "%u", pRoute->pAttr->uLocalPref);
    break;
  case 'P':
    path_dump(pStream, pRoute->pAttr->pASPathRef, 1);
    break;
  case 'o':
    switch (pRoute->pAttr->tOrigin) {
    case ROUTE_ORIGIN_IGP: log_printf(pStream, "IGP"); break;
    case ROUTE_ORIGIN_EGP: log_printf(pStream, "EGP"); break;
    case ROUTE_ORIGIN_INCOMPLETE: log_printf(pStream, "INCOMPLETE"); break;
    default:
      log_printf(pStream, "???");
    }
    break;
  case 'm':
    log_printf(pStream, "%u", pRoute->pAttr->uMED);
    break;
  case 'n':
    ip_address_dump(pStream, pRoute->pAttr->tNextHop);
    break;
  case 'c':
    if (pRoute->pAttr->pCommunities != NULL)
      comm_dump(pStream, pRoute->pAttr->pCommunities, COMM_DUMP_RAW);
    break;
  case 'e':
    if (pRoute->pAttr->pECommunities != NULL)
      ecomm_dump(pStream, pRoute->pAttr->pECommunities, ECOMM_DUMP_RAW);
    break;
  case 'O':
    if (pRoute->pAttr->pOriginator != NULL)
      ip_address_dump(pStream, *pRoute->pAttr->pOriginator);
    break;
  case 'C':
    if (pRoute->pAttr->pClusterList != NULL)
      cluster_list_dump(pStream, pRoute->pAttr->pClusterList);
    break;
  case 'A':
    iASN= path_first_as(pRoute->pAttr->pASPathRef);
    if (iASN >= 0) {
      log_printf(pStream, "%u", iASN);
    } else {
      log_printf(pStream, "*");
    }
    break;
  default:
    return -1;
  }
  return 0;
}

void route_dump_custom(SLogStream * pStream, SRoute * pRoute,
		       const char * pcFormat)
{
  int iResult;
  iResult= str_format_for_each(pStream, _route_dump_custom, pRoute, pcFormat);
  assert(iResult == 0);
}

