// ==================================================================
// @(#)attr.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/11/2005
// @lastdate 13/03/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <bgp/attr.h>
#include <bgp/comm_hash.h>
#include <bgp/path_hash.h>

// -----[ Forward prototypes declaration ]---------------------------
/* Note: functions starting with underscore (_) are intended to be
 * used inside this file only (private). These functions should be
 * static and should not appear in the .h file.
 */
static void _bgp_attr_path_copy(SBGPAttr * pAttr, SBGPPath * pPath);
static void _bgp_attr_path_destroy(SBGPAttr * pAttr);
static void _bgp_attr_comm_copy(SBGPAttr * pAttr,
				SCommunities * pCommunities);
static void _bgp_attr_ecomm_copy(SBGPAttr * pAttr,
				 SECommunities * pCommunities);
static void _bgp_attr_ecomm_destroy(SBGPAttr * pAttr);

// -----[ bgp_attr_create ]------------------------------------------
/**
 *
 */
SBGPAttr * bgp_attr_create(net_addr_t tNextHop,
			   bgp_origin_t tOrigin,
			   uint32_t uLocalPref,
			   uint32_t uMED)
{
  SBGPAttr * pAttr= (SBGPAttr *) MALLOC(sizeof(SBGPAttr));
  pAttr->tNextHop= tNextHop;
  pAttr->tOrigin= tOrigin;
  pAttr->uLocalPref= uLocalPref;
  pAttr->uMED= uMED;
  pAttr->pASPathRef= NULL;
  pAttr->pCommunities= NULL;
  pAttr->pECommunities= NULL;

  /* Route-reflection related fields */
  pAttr->pOriginator= NULL;
  pAttr->pClusterList= NULL;

  return pAttr;
}

// -----[ bgp_attr_destroy ]-----------------------------------------
/**
 *
 */
void bgp_attr_destroy(SBGPAttr ** ppAttr)
{
  if (*ppAttr != NULL) {
    _bgp_attr_path_destroy(*ppAttr);
    bgp_attr_comm_destroy(ppAttr);
    _bgp_attr_ecomm_destroy(*ppAttr);

    /* Route-reflection */
    bgp_attr_originator_destroy(*ppAttr);
    bgp_attr_cluster_list_destroy(*ppAttr);
    FREE(*ppAttr);
  }
}

// -----[ bgp_attr_set_nexthop ]-------------------------------------
/**
 *
 */
void bgp_attr_set_nexthop(SBGPAttr ** ppAttr,
				 net_addr_t tNextHop)
{
  (*ppAttr)->tNextHop= tNextHop;
}

// -----[ bgp_attr_get_nexthop ]-------------------------------------
/**
 *
 */
net_addr_t bgp_attr_get_nexthop(SBGPAttr * pAttr)
{
  return pAttr->tNextHop;
}

// -----[ bgp_attr_set_origin ]--------------------------------------
/**
 *
 */
void bgp_attr_set_origin(SBGPAttr ** ppAttr,
				      bgp_origin_t tOrigin)
{
  (*ppAttr)->tOrigin= tOrigin;
}

// -----[ bgp_attr_set_path ]----------------------------------------
/**
 * Takes an external path and associates it to the route. If the path
 * already exists in the repository, the external path is destroyed.
 * Otherwize, it is internalized (put in the repository).
 */
void bgp_attr_set_path(SBGPAttr ** ppAttr, SBGPPath * pPath)
{
  _bgp_attr_path_destroy(*ppAttr);
  if (pPath != NULL) {
    assert(path_hash_add(pPath) != -1);
    (*ppAttr)->pASPathRef= path_hash_get(pPath);

    if ((*ppAttr)->pASPathRef != pPath)
      path_destroy(&pPath);
  }
}

// -----[ bgp_attr_path_prepend ]------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 */
int bgp_attr_path_prepend(SBGPAttr ** ppAttr, uint16_t uAS, uint8_t uAmount)
{
  SBGPPath * pPath;

  if ((*ppAttr)->pASPathRef == NULL)
    pPath= path_create();
  else
    pPath= path_copy((*ppAttr)->pASPathRef);
  while (uAmount-- > 0) {
    if (path_append(&pPath, uAS) < 0)
      return -1;
  }
  bgp_attr_set_path(ppAttr, pPath);
  return 0;
}

// -----[ _bgp_attr_path_copy ]--------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 */
static void _bgp_attr_path_copy(SBGPAttr * pAttr, SBGPPath * pPath)
{
  
  _bgp_attr_path_destroy(pAttr);
  bgp_attr_set_path(&pAttr, path_copy(pPath));
}

// -----[ _bgp_attr_path_destroy ]-----------------------------------
/**
 * Destroy the AS-Path and removes the global reference.
 */
static void _bgp_attr_path_destroy(SBGPAttr * pAttr)
{
  if (pAttr->pASPathRef != NULL) {
    /*log_printf(pLogErr, "remove (%p)\n", pAttr->pASPathRef);*/
    path_hash_remove(pAttr->pASPathRef);
    pAttr->pASPathRef= NULL;
  }
}

// -----[ bgp_attr_set_comm ]----------------------------------------
/**
 *
 */
void bgp_attr_set_comm(SBGPAttr ** ppAttr,
			      SCommunities * pCommunities)
{
  bgp_attr_comm_destroy(ppAttr);
  if (pCommunities != NULL) {
    assert(comm_hash_add(pCommunities) != -1);
    (*ppAttr)->pCommunities= comm_hash_get(pCommunities);
    if ((*ppAttr)->pCommunities != pCommunities)
      comm_destroy(&pCommunities);
  }
}

// -----[ _bgp_attr_comm_copy ]--------------------------------------
/**
 * Copy the Communities attribute and update the references into the
 * global path repository.
 */
void _bgp_attr_comm_copy(SBGPAttr * pAttr,
			       SCommunities * pCommunities)
{
  bgp_attr_comm_destroy(&pAttr);
  if (pCommunities != NULL)
    bgp_attr_set_comm(&pAttr, comm_copy(pCommunities));
}

// -----[ bgp_attr_comm_destroy ]-----------------------------------
/**
 * Destroy the Communities attribute and removes the global
 * reference.
 */
void bgp_attr_comm_destroy(SBGPAttr ** ppAttr)
{
  if ((*ppAttr)->pCommunities != NULL) {
    comm_hash_remove((*ppAttr)->pCommunities);
    (*ppAttr)->pCommunities= NULL;
  }
}

// -----[ bgp_attr_comm_remove ]-------------------------------------
/**
 *
 */
void bgp_attr_comm_remove(SBGPAttr ** ppAttr, comm_t tCommunity)
{
  SCommunities * pCommunities;

  if ((*ppAttr)->pCommunities == NULL)
    return;
  pCommunities= comm_copy((*ppAttr)->pCommunities);
  comm_remove(&pCommunities, tCommunity);
  bgp_attr_set_comm(ppAttr, pCommunities);
}

// -----[ _bgp_attr_ecomm_copy ]-------------------------------------
static void _bgp_attr_ecomm_copy(SBGPAttr * pAttr,
				 SECommunities * pCommunities)
{
  _bgp_attr_ecomm_destroy(pAttr);
  if (pCommunities != NULL)
    pAttr->pECommunities= ecomm_copy(pCommunities);
}

// -----[ _bgp_attr_ecomm_destroy ]----------------------------------
/**
 * Destroy the extended-communities attribute.
 */
static void _bgp_attr_ecomm_destroy(SBGPAttr * pAttr)
{
  ecomm_destroy(&pAttr->pECommunities);
}

// -----[ _bgp_attr_originator_copy ]--------------------------------
/**
 *
 */
static inline void _bgp_attr_originator_copy(SBGPAttr * pAttr,
					     net_addr_t * pOriginator)
{
  if (pOriginator == NULL)
    pAttr->pOriginator= NULL;
  else {
    pAttr->pOriginator= (net_addr_t *) MALLOC(sizeof(net_addr_t));
    *pAttr->pOriginator= *pOriginator;
  }
}

// -----[ bgp_attr_originator_destroy ]------------------------------
/**
 *
 */
void bgp_attr_originator_destroy(SBGPAttr * pAttr)
{
  if (pAttr->pOriginator != NULL) {
    FREE(pAttr->pOriginator);
    pAttr->pOriginator= NULL;
  }
}

// -----[ _bgp_attr_cluster_list_copy ]-------------------------------
/**
 *
 */
static inline void _bgp_attr_cluster_list_copy(SBGPAttr * pAttr,
					       SClusterList * pClusterList)
{
  if (pClusterList == NULL)
    pAttr->pClusterList= NULL;
  else
    pAttr->pClusterList= cluster_list_copy(pClusterList);
}

// -----[ bgp_attr_cluster_list_destroy ]----------------------------
/**
 *
 */
void bgp_attr_cluster_list_destroy(SBGPAttr * pAttr)
{
  cluster_list_destroy(&pAttr->pClusterList);
}

// -----[ bgp_attr_copy ]--------------------------------------------
/**
 *
 */
SBGPAttr * bgp_attr_copy(SBGPAttr * pAttr)
{
  SBGPAttr * pAttrCopy= bgp_attr_create(pAttr->tNextHop,
					pAttr->tOrigin,
					pAttr->uLocalPref,
					pAttr->uMED);
  _bgp_attr_path_copy(pAttrCopy, pAttr->pASPathRef);
  _bgp_attr_comm_copy(pAttrCopy, pAttr->pCommunities);
  _bgp_attr_ecomm_copy(pAttrCopy, pAttr->pECommunities);
  /* Route-Reflection attributes */
  _bgp_attr_originator_copy(pAttrCopy, pAttr->pOriginator);
  _bgp_attr_cluster_list_copy(pAttrCopy, pAttr->pClusterList);
  return pAttrCopy;
}

// -----[ bgp_attr_cmp ]---------------------------------------------
/**
 * This function compares two sets of BGP attributes. The function
 * only tests if the values of both sets are equal or not.
 *
 * Return value:
 *   1  if attributes are equal
 *   0  otherwise
 */
int bgp_attr_cmp(SBGPAttr * pAttr1, SBGPAttr * pAttr2)
{
  // If both pointers are equal, the content must also be equal.
  if (pAttr1 == pAttr2)
    return 1;

  // NEXT-HOP attributes must be equal
  if (pAttr1->tNextHop != pAttr2->tNextHop) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different NEXT-HOP\n");
    return 0;
  }

  // LOCAL-PREF attributes must be equal
  if (pAttr1->uLocalPref != pAttr2->uLocalPref) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different LOCAL-PREFs\n");
    return 0;
  }

  // AS-PATH attributes must be equal
  if (!path_equals(pAttr1->pASPathRef, pAttr2->pASPathRef)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different AS-PATHs\n");
    return 0;
  }

  // ORIGIN attributes must be equal
  if (pAttr1->tOrigin != pAttr2->tOrigin) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different ORIGINs\n");
    return 0;
  }

  // MED attributes must be equal
  if (pAttr1->uMED != pAttr2->uMED) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different MEDs\n");
    return 0;
  }

  // COMMUNITIES attributes must be equal
  if (!comm_equals(pAttr1->pCommunities, pAttr2->pCommunities)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different COMMUNITIES\n");
    return 0;
  }

  // EXTENDED-COMMUNITIES attributes must be equal
  if (!ecomm_equals(pAttr1->pECommunities, pAttr2->pECommunities)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different EXTENDED-COMMUNITIES\n");
    return 0;
  }

  // ORIGINATOR-ID attributes must be equal
  if (!originator_equals(pAttr1->pOriginator, pAttr2->pOriginator)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different ORIGINATOR-ID\n");
    return 0;
  }
 
  // CLUSTER-ID-LIST attributes must be equal
  if (!cluster_list_equals(pAttr1->pClusterList, pAttr2->pClusterList)) {
    LOG_DEBUG(LOG_LEVEL_DEBUG, "different CLUSTER-ID-LIST\n");
    return 0;
  }
  
  return 1;
}

