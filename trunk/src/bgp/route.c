// ==================================================================
// @(#)route.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/11/2002
// @lastdate 13/08/2004
// ==================================================================

#include <assert.h>
#include <string.h>

#include <libgds/array.h>
#include <libgds/memory.h>

#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/qos.h>
#include <bgp/route.h>

unsigned long rt_create_count= 0;
unsigned long rt_copy_count= 0;
unsigned long rt_destroy_count= 0;

// ----- route_create -----------------------------------------------
/**
 *
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
  pRoute->pASPath= NULL;
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

  return pRoute;
}

// ----- route_destroy ----------------------------------------------
/**
 *
 */
void route_destroy(SRoute ** ppRoute)
{
  if (*ppRoute != NULL) {
    rt_destroy_count++;
    path_destroy(&(*ppRoute)->pASPath);
    route_comm_strip(*ppRoute);
    ecomm_destroy(&(*ppRoute)->pECommunities);

    /* BGP QoS */
#ifdef BGP_QOS
    ptr_array_destroy(&(*ppRoute)->pAggrRoutes);
#endif

    /* Route-reflection */
    route_originator_clear(*ppRoute);
    route_cluster_list_clear(*ppRoute);

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

// ----- route_nexthop_set ------------------------------------------
/**
 *
 */
void route_nexthop_set(SRoute * pRoute,
		       net_addr_t tNextHop)
{
  pRoute->tNextHop= tNextHop;
}

// ----- route_nexthop_get ------------------------------------------
/**
 *
 */
net_addr_t route_nexthop_get(SRoute * pRoute)
{
  return pRoute->tNextHop;
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

// ----- route_origin_set -------------------------------------------
/**
 *
 */
void route_origin_set(SRoute * pRoute, origin_type_t uOriginType)
{
  pRoute->uOriginType= uOriginType;
}

// ----- route_origin_get -------------------------------------------
/**
 *
 */
origin_type_t route_origin_get(SRoute * pRoute)
{
  return pRoute->uOriginType;
}

// ----- route_path_append ------------------------------------------
/**
 *
 */
int route_path_append(SRoute * pRoute, uint16_t uAS)
{
  if (pRoute->pASPath == NULL)
    pRoute->pASPath= path_create();
  return path_append(&pRoute->pASPath, uAS);
}

// ----- route_path_prepend -----------------------------------------
/**
 *
 */
int route_path_prepend(SRoute * pRoute, uint16_t uAS, uint8_t uAmount)
{
  while (uAmount-- > 0) {
    if (route_path_append(pRoute, uAS))
      return -1;
  }
  return 0;
}


// ----- route_path_contains ----------------------------------------
/**
 *
 */
int route_path_contains(SRoute * pRoute, uint16_t uAS)
{
  return (pRoute->pASPath != NULL) &&
    (path_contains(pRoute->pASPath, uAS) != -1);
}

// ----- route_path_length ------------------------------------------
/**
 *
 */
int route_path_length(SRoute * pRoute)
{
  if (pRoute->pASPath != NULL)
    return path_length(pRoute->pASPath);
  else
    return 0;
}

// ----- route_comm_append ------------------------------------------
/**
 *
 */
int route_comm_append(SRoute * pRoute, comm_t uCommunity)
{
  if (pRoute->pCommunities == NULL)
    pRoute->pCommunities= comm_create();
  return comm_add(pRoute->pCommunities, uCommunity);
}

// ----- route_comm_strip -------------------------------------------
/**
 *
 */
void route_comm_strip(SRoute * pRoute)
{
  comm_destroy(&pRoute->pCommunities);
}

// ----- route_comm_remove ------------------------------------------
/**
 *
 */
void route_comm_remove(SRoute * pRoute, comm_t uCommunity)
{
  if (pRoute->pCommunities != NULL)
    comm_remove(pRoute->pCommunities, uCommunity);
}

// ----- route_comm_contains ----------------------------------------
/**
 *
 */
int route_comm_contains(SRoute * pRoute, comm_t uCommunity)
{
  return (pRoute->pCommunities != NULL) &&
    (comm_contains(pRoute->pCommunities, uCommunity) != -1);
}

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
  pNewRoute->tNextHop= pRoute->tNextHop;
  if (pRoute->pASPath != NULL)
    pNewRoute->pASPath= path_copy(pRoute->pASPath);
  if (pRoute->pCommunities != NULL)
    pNewRoute->pCommunities= sequence_copy(pRoute->pCommunities, NULL);
  pNewRoute->uLocalPref= pRoute->uLocalPref;
  pNewRoute->uMED= pRoute->uMED;
  if (pRoute->pECommunities != NULL)
    pNewRoute->pECommunities= ecomm_copy(pRoute->pECommunities);
  pNewRoute->uTBID= pRoute->uTBID;
  pNewRoute->uFlags= pRoute->uFlags;

  /* BGP QoS */
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

  /* Route-Reflection: Originator and Cluster-ID-List fields */
  if (route_originator_get(pRoute, &tOriginator) == 0)
    route_originator_set(pNewRoute, tOriginator);
  pNewRoute->pClusterList= route_cluster_list_copy(pRoute);

  return pNewRoute;
}

// ----- route_dump -------------------------------------------------
/**
 * CISCO-like dump of routes.
 */
void route_dump(FILE * pStream, SRoute * pRoute)
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
    path_dump(pStream, pRoute->pASPath, 1);

    // Origin
    fprintf(pStream, "\t");
    switch (pRoute->uOriginType) {
    case ROUTE_ORIGIN_IGP: fprintf(pStream, "i"); break;
    case ROUTE_ORIGIN_EGP: fprintf(pStream, "e"); break;
    case ROUTE_ORIGIN_INCOMPLETE: fprintf(pStream, "?"); break;
    }

  } else
    fprintf(pStream, "(null)");
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
    cCharTmp = path_dump_string(pRoute->pASPath, 1);
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

// ----- route_dump_mrtd --------------------------------------------
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
void route_dump_mrtd(FILE * pStream, SRoute * pRoute)
{
  if (pRoute == NULL) {
    fprintf(pStream, "null");
  } else {
    // BEST flag
    if (route_flag_get(pRoute, ROUTE_FLAG_BEST)) 
      fprintf(pStream, "B|");
    else
      fprintf(pStream, "?|");
    // Peer-IP
    if (pRoute->pPeer != NULL) {
      ip_address_dump(pStream, route_peer_get(pRoute)->tAddr);
      // Peer-AS
      fprintf(pStream, "|%u|", route_peer_get(pRoute)->uRemoteAS);
    } else
      fprintf(pStream, "LOCAL|LOCAL|");
    // Prefix
    ip_prefix_dump(pStream, pRoute->sPrefix);
    fprintf(pStream, "|");
    // AS-Path
    path_dump(pStream, pRoute->pASPath, 1);
    fprintf(pStream, "|");
    // Origin
    switch (pRoute->uOriginType) {
    case ROUTE_ORIGIN_IGP: fprintf(pStream, "IGP"); break;
    case ROUTE_ORIGIN_EGP: fprintf(pStream, "EGP"); break;
    case ROUTE_ORIGIN_INCOMPLETE: fprintf(pStream, "INCOMPLETE"); break;
    default:
      fprintf(pStream, "???");
    }
    fprintf(pStream, "|");
    // Next-Hop
    ip_address_dump(pStream, pRoute->tNextHop);
    // Local-Pref
    fprintf(pStream, "|%u|", pRoute->uLocalPref);
    // Multi-Exit-Discriminator
    if (pRoute->uMED != ROUTE_MED_MISSING)
      fprintf(pStream, "%u|", pRoute->uMED);
    else
      fprintf(pStream, "|");
    // Communities
    if (pRoute->pCommunities != NULL)
      comm_dump(pStream, pRoute->pCommunities);
    fprintf(pStream, "|");
    // Extended-Communities
    if (pRoute->pECommunities != NULL)
      ecomm_dump(pStream, pRoute->pECommunities);
    fprintf(pStream, "|");

    /*
    // QoS Delay
    fprintf(pStream, "%u|%u|%u|", pRoute->tDelay.tDelay,
	    pRoute->tDelay.tMean, pRoute->tDelay.uWeight);
    // QoS Bandwidth
    fprintf(pStream, "%u|%u|%u|", pRoute->tBandwidth.uBandwidth,
	    pRoute->tBandwidth.uMean, pRoute->tBandwidth.uWeight);
    // Route-reflectors: Originator
    if (pRoute->pOriginator != NULL)
      ip_address_dump(pStream, *pRoute->pOriginator);
    fprintf(pStream, "|");
    if (pRoute->pClusterList != NULL)
      cluster_list_dump(pStream, pRoute->pClusterList);
    fprintf(pStream, "|");
    */
  }
}

// ----- route_equals -----------------------------------------------
/**
 *
 */
int route_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  if (pRoute1 == pRoute2) {
    LOG_DEBUG("route_equals == 1\n");
    return 1;
  }

  /* BGP QoS */
#ifdef BGP_QOS
  if (!qos_route_delay_equals(pRoute1, pRoute2) ||
      !qos_route_bandwidth_equals(pRoute1, pRoute2)) {
    LOG_DEBUG("route_equals == 0\n");
    return 0;
  }
#endif

  if ((ip_prefix_equals(pRoute1->sPrefix, pRoute2->sPrefix)) &&
      (pRoute1->pPeer == pRoute2->pPeer) &&
      (pRoute1->uOriginType == pRoute2->uOriginType) &&
      (path_equals(pRoute1->pASPath, pRoute2->pASPath)) &&
      (comm_equals(pRoute1->pCommunities, pRoute2->pCommunities)) &&
      (pRoute1->uLocalPref == pRoute2->uLocalPref) &&
      (pRoute1->uMED == pRoute2->uMED) &&
      (ecomm_equals(pRoute1->pECommunities, pRoute2->pECommunities)) &&
      (pRoute1->uTBID == pRoute2->uTBID) &&
      (route_originator_equals(pRoute1, pRoute2)) &&
      (route_cluster_list_equals(pRoute1, pRoute2))) {
    LOG_DEBUG("route_equals == 1\n");
    return 1;
  }
    LOG_DEBUG("route_equals == 0\n");
  return 0;
}
