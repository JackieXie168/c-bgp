// ==================================================================
// @(#)route.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/11/2002
// @lastdate 04/11/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/array.h>
#include <libgds/memory.h>

#include <bgp/comm.h>
#include <bgp/comm_hash.h>
#include <bgp/ecomm.h>
#include <bgp/path_hash.h>
#include <bgp/qos.h>
#include <bgp/route.h>

unsigned long rt_create_count= 0;
unsigned long rt_copy_count= 0;
unsigned long rt_destroy_count= 0;

// -----[ Forward prototypes declaration ]---------------------------
/* Note: functions starting with underscore (_) are intended to be
 * used inside this file only (private). These functions should be
 * static and should not appear in the .h file.
 */
static void _route_path_copy(SRoute * pRoute, SBGPPath * pPath);
static void _route_path_destroy(SRoute * pRoute);
static void _route_comm_copy(SRoute * pRoute, SCommunities * pCommunities);
static void _route_comm_destroy(SRoute * pRoute);
static void _route_ecomm_copy(SRoute * pRoute, SECommunities * pCommunities);
static void _route_ecomm_destroy(SRoute * pRoute);

// ----- route_create -----------------------------------------------
/**
 * Create a new BGP route towards the given prefix. The 'peer' field
 * mentions the peer who announced the route to the local router. This
 * field can be NULL if the route was originated locally or if the
 * peer is unknown. The 'next-hop' field is the BGP nexth-op of the
 * route. Finally, the 'origin-type' field tells how the route was
 * originated. This is usually set to IGP.
 */
SRoute * route_create(SPrefix sPrefix, SPeer * pPeer,
		      net_addr_t tNextHop,
		      origin_type_t uOriginType)
{
  SRoute * pRoute= (SRoute *) MALLOC(sizeof(SRoute));
  rt_create_count++;
  pRoute->sPrefix= sPrefix;
  pRoute->pPeer= pPeer;
  pRoute->tNextHop= tNextHop;
  pRoute->uOriginType= uOriginType;
  pRoute->pASPathRef= NULL;
  pRoute->uLocalPref= ROUTE_PREF_DEFAULT;
  pRoute->uMED= ROUTE_MED_DEFAULT;
  pRoute->pCommunities= NULL;
  pRoute->pECommunities= NULL;
  pRoute->uFlags= 0;

  /* QoS fields */
#ifdef BGP_QOS
  qos_route_init_delay(pRoute);
  qos_route_init_bandwidth(pRoute);
  pRoute->pAggrRoutes= NULL;
  pRoute->pAggrRoute= NULL;
  pRoute->pEBGPRoute= NULL;
#endif

  /* Route-reflection related fields */
  pRoute->pOriginator= NULL;
  pRoute->pClusterList= NULL;

#ifdef __ROUTER_LIST_ENABLE__
  pRoute->pRouterList= NULL;
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
    rt_destroy_count++;
    _route_path_destroy(*ppRoute);
    _route_comm_destroy(*ppRoute);
    _route_ecomm_destroy(*ppRoute);

    /* BGP QoS */
#ifdef BGP_QOS
    ptr_array_destroy(&(*ppRoute)->pAggrRoutes);
#endif

    /* Route-reflection */
    route_originator_clear(*ppRoute);
    route_cluster_list_clear(*ppRoute);

#ifdef __ROUTER_LIST_ENABLE__
    route_router_list_clear(*ppRoute);
#endif

    FREE(*ppRoute);
    *ppRoute= NULL;
  }
}

// ----- route_flag_set ---------------------------------------------
/**
 *
 */
void route_flag_set(SRoute * pRoute, uint16_t uFlag, int iState)
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
int route_flag_get(SRoute * pRoute, uint16_t uFlag)
{
  return pRoute->uFlags & uFlag;
}

// ----- route_peer_set ---------------------------------------------
/**
 *
 */
void route_peer_set(SRoute * pRoute, SPeer * pPeer)
{
  pRoute->pPeer= pPeer;
}

// ----- route_peer_get ---------------------------------------------
/**
 *
 */
SPeer * route_peer_get(SRoute * pRoute)
{
  assert(pRoute->pPeer != NULL);
  return pRoute->pPeer;
}

/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE NEXT-HOP
//
/////////////////////////////////////////////////////////////////////

// ----- route_nexthop_set ------------------------------------------
/**
 * Set the route's BGP next-hop.
 */
void route_nexthop_set(SRoute * pRoute,
		       net_addr_t tNextHop)
{
  pRoute->tNextHop= tNextHop;
}

// ----- route_nexthop_get ------------------------------------------
/**
 * Return the route's BGP next-hop.
 */
net_addr_t route_nexthop_get(SRoute * pRoute)
{
  return pRoute->tNextHop;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGIN
//
/////////////////////////////////////////////////////////////////////

// ----- route_origin_set -------------------------------------------
/**
 * Set the route's origin.
 */
void route_origin_set(SRoute * pRoute, origin_type_t uOriginType)
{
  pRoute->uOriginType= uOriginType;
}

// ----- route_origin_get -------------------------------------------
/**
 * Return the route's origin.
 */
origin_type_t route_origin_get(SRoute * pRoute)
{
  return pRoute->uOriginType;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE AS-PATH
//
/////////////////////////////////////////////////////////////////////

// -----[ route_path_set ]-------------------------------------------
/**
 * Set the route's AS-Path and adds a global reference if required.
 *
 * Note: the given AS-Path is not copied, but it will be referenced in
 * the global path repository if used or destroyed if not.
 */
void route_path_set(SRoute * pRoute, SBGPPath * pPath)
{
  _route_path_destroy(pRoute);
  if (pPath != NULL) {
    assert(path_hash_add(pPath) != -1);
    pRoute->pASPathRef= path_hash_get(pPath);
    if (pRoute->pASPathRef != pPath)
      path_destroy(&pPath);
  }
}

// -----[ route_path_get ]-------------------------------------------
/**
 * Return the route's AS-Path.
 */
SBGPPath * route_path_get(SRoute * pRoute)
{
  return pRoute->pASPathRef;
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
 *  0   in case of success
 *  -1  in case of failure (AS-Path would become too long)
 */
int route_path_prepend(SRoute * pRoute, uint16_t uAS, uint8_t uAmount)
{
  SBGPPath * pPath;

  if (pRoute->pASPathRef == NULL)
    pPath= path_create();
  else
    pPath= path_copy(pRoute->pASPathRef);
  while (uAmount-- > 0) {
    if (path_append(&pPath, uAS) < 0)
      return -1;
  }
  route_path_set(pRoute, pPath);
  return 0;
}

// ----- route_path_contains ----------------------------------------
/**
 * Test if the route's AS-PAth contains the given AS-number.
 *
 * Returns 1 if true, 0 otherwise.
 */
int route_path_contains(SRoute * pRoute, uint16_t uAS)
{
  return (pRoute->pASPathRef != NULL) &&
    (path_contains(pRoute->pASPathRef, uAS) != -1);
}

// ----- route_path_length ------------------------------------------
/**
 * Return the length of the route's AS-Path.
 */
int route_path_length(SRoute * pRoute)
{
  if (pRoute->pASPathRef != NULL)
    return path_length(pRoute->pASPathRef);
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
int route_path_last_as(SRoute * pRoute)
{
  if (pRoute->pASPathRef != NULL)
    return path_last_as(pRoute->pASPathRef);
  else
    return -1;
}

// -----[ _route_path_copy ]-----------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 */
static void _route_path_copy(SRoute * pRoute, SBGPPath * pPath)
{
  _route_path_destroy(pRoute);
  route_path_set(pRoute, path_copy(pPath));
}

// -----[ _route_path_destroy ]--------------------------------------
/**
 * Destroy the route's AS-Path and removes the global reference.
 */
static void _route_path_destroy(SRoute * pRoute)
{
  if (pRoute->pASPathRef != NULL) {
    path_hash_remove(pRoute->pASPathRef);
    pRoute->pASPathRef= NULL;
  }
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// ----- route_comm_set ---------------------------------------------
/**
 * Set the route's Communities attribute and adds a global reference
 * if required.
 *
 * Note: the given Communities is not copied, but it will be
 * referenced in the global Communities repository if used or
 * destroyed if not.
 */
void route_comm_set(SRoute * pRoute, SCommunities * pCommunities)
{
  _route_comm_destroy(pRoute);
  if (pCommunities != NULL) {
    assert(comm_hash_add(pCommunities) != -1);
    pRoute->pCommunities= comm_hash_get(pCommunities);
    if (pRoute->pCommunities != pCommunities)
      comm_destroy(&pCommunities);
  }
}

// ----- route_comm_append ------------------------------------------
/**
 * Add a new community value to the Communities attribute.
 */
int route_comm_append(SRoute * pRoute, comm_t tCommunity)
{
  SCommunities * pCommunities;

  if (pRoute->pCommunities == NULL)
    pCommunities= comm_create();
  else
    pCommunities= comm_copy(pRoute->pCommunities);
  if (comm_add(pCommunities, tCommunity)) {
    comm_destroy(&pCommunities);
    return -1;
  }
  route_comm_set(pRoute, pCommunities);
  return 0;
}

// ----- route_comm_strip -------------------------------------------
/**
 * Remove all the community values from the Communities attribute.
 */
void route_comm_strip(SRoute * pRoute)
{
  _route_comm_destroy(pRoute);
}

// ----- route_comm_remove ------------------------------------------
/**
 * Remove a single community value from the Communities attribute.
 */
void route_comm_remove(SRoute * pRoute, comm_t tCommunity)
{
  SCommunities * pCommunities;

  if (pRoute->pCommunities == NULL)
    return;
  pCommunities= comm_copy(pRoute->pCommunities);
  comm_remove(pCommunities, tCommunity);
  route_comm_set(pRoute, pCommunities);
}

// ----- route_comm_contains ----------------------------------------
/**
 * Test if the Communities attribute contains the given community
 * value.
 */
int route_comm_contains(SRoute * pRoute, comm_t tCommunity)
{
  return (pRoute->pCommunities != NULL) &&
    (comm_contains(pRoute->pCommunities, tCommunity) != -1);
}

// -----[ _route_comm_copy ]-----------------------------------------
/**
 * Copy the Communities attribute and update the references into the
 * global path repository.
 */
static void _route_comm_copy(SRoute * pRoute, SCommunities * pCommunities)
{
  _route_comm_destroy(pRoute);
  if (pCommunities != NULL)
    route_comm_set(pRoute, comm_copy(pCommunities));
}

// -----[ _route_comm_destroy ]--------------------------------------
/**
 * Destroy the Communities attribute and removes the global
 * reference.
 */
static void _route_comm_destroy(SRoute * pRoute)
{
  if (pRoute->pCommunities != NULL) {
    comm_hash_remove(pRoute->pCommunities);
    pRoute->pCommunities= NULL;
  }
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
int route_ecomm_append(SRoute * pRoute, SECommunity * pComm)
{
  if (pRoute->pECommunities == NULL)
    pRoute->pECommunities= ecomm_create();
  return ecomm_add(&pRoute->pECommunities, pComm);
}

// ----- route_ecomm_strip_non_transitive ---------------------------
/**
 *
 */
void route_ecomm_strip_non_transitive(SRoute * pRoute)
{
  ecomm_strip_non_transitive(&pRoute->pECommunities);
}

// -----[ _route_ecomm_copy ]----------------------------------------
static void _route_ecomm_copy(SRoute * pRoute, SECommunities * pCommunities)
{
  _route_ecomm_destroy(pRoute);
  if (pCommunities != NULL)
    pRoute->pECommunities= ecomm_copy(pCommunities);
}

// -----[ _route_ecomm_destroy ]-------------------------------------
/**
 * Destroy the route's extended-communities.
 */
static void _route_ecomm_destroy(SRoute * pRoute)
{
  ecomm_destroy(&pRoute->pECommunities);
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
void route_localpref_set(SRoute * pRoute, uint32_t uPref)
{
  pRoute->uLocalPref= uPref;
}

// ----- route_localpref_get ----------------------------------------
/**
 *
 */
uint32_t route_localpref_get(SRoute * pRoute)
{
  return pRoute->uLocalPref;
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
void route_med_clear(SRoute * pRoute)
{
  pRoute->uMED= ROUTE_MED_MISSING;
}

// ----- route_med_set ----------------------------------------------
/**
 *
 */
void route_med_set(SRoute * pRoute, uint32_t uMED)
{
  pRoute->uMED= uMED;
}

// ----- route_med_get ----------------------------------------------
/**
 *
 */
uint32_t route_med_get(SRoute * pRoute)
{
  return pRoute->uMED;
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
void route_originator_set(SRoute * pRoute, net_addr_t tOriginator)
{
  assert(pRoute->pOriginator == NULL);
  pRoute->pOriginator= (net_addr_t *) MALLOC(sizeof(net_addr_t));
  memcpy(pRoute->pOriginator, &tOriginator, sizeof(net_addr_t));
}

// ----- route_originator_get ---------------------------------------
/**
 *
 */
int route_originator_get(SRoute * pRoute, net_addr_t * pOriginator)
{
  if (pRoute->pOriginator != NULL) {
    if (pOriginator != NULL)
      memcpy(pOriginator, pRoute->pOriginator, sizeof(net_addr_t));
    return 0;
  } else
    return -1;
}

// ----- route_originator_clear -------------------------------------
/**
 *
 */
void route_originator_clear(SRoute * pRoute)
{
  if (pRoute->pOriginator != NULL) {
    FREE(pRoute->pOriginator);
    pRoute->pOriginator= NULL;
  }
}

// ----- route_originator_equals ------------------------------------
/**
 *
 */
int route_originator_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  if (pRoute1->pOriginator == pRoute2->pOriginator)
    return 1;
  if ((pRoute1->pOriginator == NULL) || (pRoute2->pOriginator == NULL) ||
      (*pRoute1->pOriginator != *pRoute2->pOriginator))
      return 0;
  return 1;
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
void route_cluster_list_set(SRoute * pRoute)
{
  assert(pRoute->pClusterList == NULL);
  pRoute->pClusterList= cluster_list_create();
}

// ----- route_cluster_list_append ----------------------------------
/**
 *
 */
void route_cluster_list_append(SRoute * pRoute, cluster_id_t tClusterID)
{
  if (pRoute->pClusterList == NULL)
    route_cluster_list_set(pRoute);
  cluster_list_append(pRoute->pClusterList, tClusterID);
}

// ----- route_cluster_list_clear -----------------------------------
/**
 *
 */
void route_cluster_list_clear(SRoute * pRoute)
{
  cluster_list_destroy(&pRoute->pClusterList);
}

// ----- route_cluster_list_equals ----------------------------------
/**
 *
 */
int route_cluster_list_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  if (pRoute1->pClusterList == pRoute2->pClusterList)
    return 1;
  if ((pRoute1->pClusterList == NULL) || (pRoute2->pClusterList == NULL)
      || (cluster_list_cmp(pRoute1->pClusterList, pRoute2->pClusterList) != 0))
    return 0;
  return 1;
}

// ----- route_cluster_list_contains --------------------------------
/**
 *
 */
int route_cluster_list_contains(SRoute * pRoute, cluster_id_t tClusterID)
{
  if (pRoute->pClusterList != NULL)
    return cluster_list_contains(pRoute->pClusterList, tClusterID);
  return 0;
}

// ----- route_cluster_list_copy ------------------------------------
/**
 *
 */
SClusterList * route_cluster_list_copy(SRoute * pRoute)
{
  if (pRoute->pClusterList == NULL)
    return NULL;
  return cluster_list_copy(pRoute->pClusterList);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ROUTER-LIST (FOR DEBUGGING PURPOSES)
//
/////////////////////////////////////////////////////////////////////

#ifdef __ROUTER_LIST_ENABLE__
// ----- route_router_list_append -----------------------------------
void route_router_list_append(SRoute * pRoute, net_addr_t tAddr)
{
  if (pRoute->pRouterList == NULL)
    pRoute->pRouterList= cluster_list_create();
  cluster_list_append(pRoute->pRouterList, tAddr);
}

// ----- route_router_list_clear ------------------------------------
void route_router_list_clear(SRoute * pRoute)
{
  cluster_list_destroy(pRoute->pRouterList);
}

// ----- route_router_list_copy -------------------------------------
SClusterList * route_router_list_copy(SRoute * pRoute)
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
    //LOG_DEBUG("route_equals == 1\n");
    return 1;
  }

  /* BGP QoS */
#ifdef BGP_QOS
  if (!qos_route_delay_equals(pRoute1, pRoute2) ||
      !qos_route_bandwidth_equals(pRoute1, pRoute2)) {
    //LOG_DEBUG("route_equals == 0\n");
    return 0;
  }
#endif

  if ((ip_prefix_equals(pRoute1->sPrefix, pRoute2->sPrefix)) &&
      (pRoute1->pPeer == pRoute2->pPeer) &&
      (pRoute1->uOriginType == pRoute2->uOriginType) &&
      (path_equals(pRoute1->pASPathRef, pRoute2->pASPathRef)) &&
      (comm_equals(pRoute1->pCommunities, pRoute2->pCommunities)) &&
      (pRoute1->uLocalPref == pRoute2->uLocalPref) &&
      (pRoute1->uMED == pRoute2->uMED) &&
      (ecomm_equals(pRoute1->pECommunities, pRoute2->pECommunities)) &&
      (route_originator_equals(pRoute1, pRoute2)) &&
      (route_cluster_list_equals(pRoute1, pRoute2))) {
    //LOG_DEBUG("route_equals == 1\n");
    return 1;
  }
  //LOG_DEBUG("route_equals == 0\n");
  return 0;
}

// ----- route_copy -------------------------------------------------
/**
 *
 */
SRoute * route_copy(SRoute * pRoute)
{
  net_addr_t tOriginator;
  SRoute * pNewRoute= route_create(pRoute->sPrefix,
				   pRoute->pPeer,
				   pRoute->tNextHop,
				   pRoute->uOriginType);

  rt_copy_count++;
  /* Route info */
  pNewRoute->uFlags= pRoute->uFlags;

  /* Route attributes */
  pNewRoute->tNextHop= pRoute->tNextHop;
  _route_path_copy(pNewRoute, pRoute->pASPathRef);
  _route_comm_copy(pNewRoute, pRoute->pCommunities);
  pNewRoute->uLocalPref= pRoute->uLocalPref;
  pNewRoute->uMED= pRoute->uMED;
  _route_ecomm_copy(pNewRoute, pRoute->pECommunities);

  /* Route-Reflection attributes */
  if (route_originator_get(pRoute, &tOriginator) == 0)
    route_originator_set(pNewRoute, tOriginator);
  pNewRoute->pClusterList= route_cluster_list_copy(pRoute);

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

  return pNewRoute;
}


/////////////////////////////////////////////////////////////////////
//
// ROUTE DUMP
//
/////////////////////////////////////////////////////////////////////

// ----- route_dump -------------------------------------------------
/**
 * Dump a BGP route.
 */
void route_dump(FILE * pStream, SRoute * pRoute)
{
  switch (BGP_OPTIONS_SHOW_MODE) {
  case ROUTE_SHOW_MRT:
    route_dump_mrt(pStream, pRoute);
    break;
  case ROUTE_SHOW_CISCO:
  default:
    route_dump_cisco(pStream, pRoute);
  }
}

// ----- route_dump_cisco -------------------------------------------
/**
 * CISCO-like dump of routes.
 */
void route_dump_cisco(FILE * pStream, SRoute * pRoute)
{
  if (pRoute != NULL) {

    // Status code:
    // '*' - valid (feasible)
    // 'i' - internal (learned through 'add network')
    if (route_flag_get(pRoute, ROUTE_FLAG_INTERNAL))
      fprintf(pStream, "i");
    else if (route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))
      fprintf(pStream, "*");
    else fprintf(pStream, " ");

    // Best ?
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST))
      fprintf(pStream, ">");
    else
      fprintf(pStream, " ");

    fprintf(pStream, " ");

    // Prefix
    ip_prefix_dump(pStream, pRoute->sPrefix);

    // Next-Hop
    fprintf(pStream, "\t");
    ip_address_dump(pStream, pRoute->tNextHop);

    // Local-Pref & Multi-Exit-Distriminator
    fprintf(pStream, "\t%u\t%u\t", pRoute->uLocalPref, pRoute->uMED);

    // AS-Path
    if (pRoute->pASPathRef != NULL)
      path_dump(pStream, pRoute->pASPathRef, 1);
    else
      fprintf(pStream, "null");

    // Origin
    fprintf(pStream, "\t");
    switch (pRoute->uOriginType) {
    case ROUTE_ORIGIN_IGP: fprintf(pStream, "i"); break;
    case ROUTE_ORIGIN_EGP: fprintf(pStream, "e"); break;
    case ROUTE_ORIGIN_INCOMPLETE: fprintf(pStream, "?"); break;
    }

#ifdef __ROUTER_LIST_ENABLE__
    // Router list
    fprintf(pStream, "\t[");
    if (pRoute->pRouterList != NULL)
      cluster_list_dump(pStream, pRoute->pRouterList);
    fprintf(pStream, "]");
#endif

  } else
    fprintf(pStream, "(null)");
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
 * otherwize. Note that the <ext-comm> and <delay> fields are not
 * standard MRTD fields.
 *
 * Additional fields are dumped which are not standard:
 *
 *   <delay>|<delay-mean>|<delay-weight>|
 *   <bandwidth>|<bandwidth-mean>|<bandwidth-weight>
 *
 */
void route_dump_mrt(FILE * pStream, SRoute * pRoute)
{
  if (pRoute == NULL) {
    fprintf(pStream, "null");
  } else {

    /* BEST flag: in MRT's route_btoa, table dumps only contain best
       routes, so that a "B" is written. In our case, when dumping an
       Adj-RIB-In, non-best routes might exist. For these routes, a
       "_" is written. */
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) {
      fprintf(pStream, "B|");
    } else {
      fprintf(pStream, "_|");
    }

    /* Peer-IP: if the route is local, the keyword LOCAL is written in
       place of the peer's IP address and AS number. */
    if (pRoute->pPeer != NULL) {
      ip_address_dump(pStream, pRoute->pPeer->tAddr);
      // Peer-AS
      fprintf(pStream, "|%u|", pRoute->pPeer->uRemoteAS);
    } else {
      fprintf(pStream, "LOCAL|LOCAL|");
    }

    /* Prefix */
    ip_prefix_dump(pStream, pRoute->sPrefix);
    fprintf(pStream, "|");

    /* AS-Path */
    path_dump(pStream, pRoute->pASPathRef, 1);
    fprintf(pStream, "|");

    /* Origin */
    switch (pRoute->uOriginType) {
    case ROUTE_ORIGIN_IGP: fprintf(pStream, "IGP"); break;
    case ROUTE_ORIGIN_EGP: fprintf(pStream, "EGP"); break;
    case ROUTE_ORIGIN_INCOMPLETE: fprintf(pStream, "INCOMPLETE"); break;
    default:
      fprintf(pStream, "???");
    }
    fprintf(pStream, "|");

    /* Next-Hop */
    ip_address_dump(pStream, pRoute->tNextHop);

    /* Local-Pref */
    fprintf(pStream, "|%u|", pRoute->uLocalPref);

    /* Multi-Exit-Discriminator */
    if (pRoute->uMED != ROUTE_MED_MISSING)
      fprintf(pStream, "%u|", pRoute->uMED);
    else
      fprintf(pStream, "|");

    /* Communities: written as numeric values */
    if (pRoute->pCommunities != NULL)
      comm_dump(pStream, pRoute->pCommunities, COMM_DUMP_RAW);
    fprintf(pStream, "|");

    /* Extended-Communities: written as numeric values */
    if (pRoute->pECommunities != NULL)
      ecomm_dump(pStream, pRoute->pECommunities, ECOMM_DUMP_RAW);
    fprintf(pStream, "|");

    // Route-reflectors: Originator-ID
    if (pRoute->pOriginator != NULL)
      ip_address_dump(pStream, *pRoute->pOriginator);
    fprintf(pStream, "|");

    // Route-reflectors: Cluster-ID-List
    if (pRoute->pClusterList != NULL)
      cluster_list_dump(pStream, pRoute->pClusterList);
    fprintf(pStream, "|");
    
#ifdef __ROUTER_LIST_ENABLE__
    // Router list
    if (pRoute->pRouterList != NULL)
      cluster_list_dump(pStream, pRoute->pRouterList);
    fprintf(pStream, "|");
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

// ----- route_dump_string ------------------------------------------
/**
 *
 */
char * route_dump_string(SRoute * pRoute)
{
  char * cDump = MALLOC(255), * cCharTmp;
  uint8_t icDumpPtr =0;

  if (pRoute != NULL) {

    // Status code:
    // '*' - valid (feasible)
    // 'i' - internal (learned through 'add network')
    if (route_flag_get(pRoute, ROUTE_FLAG_INTERNAL)) {
      strcpy(cDump, "i");
      icDumpPtr++;
    } else if (route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))
      strcpy(cDump+icDumpPtr++, "*");
    else strcpy(cDump+icDumpPtr++, " ");

    // Best ?
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST))
      strcpy(cDump+icDumpPtr++, ">");
    else
      strcpy(cDump+icDumpPtr++, " ");

    strcpy(cDump+icDumpPtr++, " ");

    // Prefix
    //ip_prefix_dump(pStream, pRoute->sPrefix);
    cCharTmp = ip_prefix_dump_string(pRoute->sPrefix);
    strcpy(cDump+icDumpPtr, cCharTmp);
    icDumpPtr += strlen(cCharTmp);
    FREE(cCharTmp);


    // Next-Hop
    strcpy(cDump+icDumpPtr++, "\t");

    cCharTmp = ip_address_dump_string(pRoute->tNextHop);
    strcpy(cDump+icDumpPtr, cCharTmp);
    icDumpPtr += strlen(cCharTmp);
    FREE(cCharTmp);

    // Local-Pref & Multi-Exit-Distriminator
    icDumpPtr += sprintf(cDump+icDumpPtr, "\t%u\t%u\t", pRoute->uLocalPref, pRoute->uMED);

    // AS-Path
    cCharTmp = path_dump_string(pRoute->pASPathRef, 1);
    strcpy(cDump+icDumpPtr, cCharTmp);
    icDumpPtr += strlen(cCharTmp);
    FREE(cCharTmp);

    // Origin
    strcpy(cDump+icDumpPtr++, "\t");
    switch (pRoute->uOriginType) {
    case ROUTE_ORIGIN_IGP: strcpy(cDump+icDumpPtr++, "i"); break;
    case ROUTE_ORIGIN_EGP: strcpy(cDump+icDumpPtr++, "e"); break;
    case ROUTE_ORIGIN_INCOMPLETE: strcpy(cDump+icDumpPtr++, "?"); break;
    }

  } else
    strcpy(cDump, "(null)");

  return cDump;
}


/////////////////////////////////////////////////////////////////////
//
// VALIDATION & TESTS
//
/////////////////////////////////////////////////////////////////////

void _route_test()
{
}
