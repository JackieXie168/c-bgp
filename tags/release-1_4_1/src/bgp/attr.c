// ==================================================================
// @(#)attr.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/11/2005
// @lastdate 20/07/2007
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
static inline void _bgp_attr_path_destroy(SBGPAttr * pAttr);
static inline void _bgp_attr_comm_destroy(SBGPAttr ** ppAttr);
static inline void _bgp_attr_ecomm_destroy(SBGPAttr * pAttr);

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


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE AS-PATH
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_path_copy ]--------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 *
 * Pre-condition:
 *   the source AS-path must be intern, that is it must already be in
 *   the global AS-Path repository.
 */
static inline void _bgp_attr_path_copy(SBGPAttr * pAttr, SBGPPath * pPath)
{
  /*log_printf(pLogErr, "-->PATH_COPY [%p]\n", pPath);*/

  pAttr->pASPathRef= path_hash_add(pPath);

  // Check that the referenced path is equal to the source path
  // (if not, that means that the source path was not intern)
  assert(pAttr->pASPathRef == pPath);

  /*log_printf(pLogErr, "<--PATH_COPY [%p]\n", pAttr->pASPathRef);*/
}

// -----[ _bgp_attr_path_destroy ]-----------------------------------
/**
 * Destroy the AS-Path and removes the global reference.
 */
static inline void _bgp_attr_path_destroy(SBGPAttr * pAttr)
{
  /*log_printf(pLogErr, "-->PATH_DESTROY [%p]\n", pAttr->pASPathRef);*/
  if (pAttr->pASPathRef != NULL) {
    /*log_printf(pLogErr, "attr-remove (%p)\n", pAttr->pASPathRef);*/
    path_hash_remove(pAttr->pASPathRef);
    pAttr->pASPathRef= NULL;
  }
  /*log_printf(pLogErr, "<--PATH_DESTROY [---]\n");*/
}

// -----[ bgp_attr_set_path ]----------------------------------------
/**
 * Takes an external path and associates it to the route. If the path
 * already exists in the repository, the external path is destroyed.
 * Otherwize, it is internalized (put in the repository).
 */
void bgp_attr_set_path(SBGPAttr ** ppAttr, SBGPPath * pPath)
{
  /*log_printf(pLogErr, "-->PATH_SET [%p]\n", pPath);*/
  _bgp_attr_path_destroy(*ppAttr);
  if (pPath != NULL) {
    (*ppAttr)->pASPathRef= path_hash_add(pPath);
    assert((*ppAttr)->pASPathRef != NULL);
    if ((*ppAttr)->pASPathRef != pPath)
      path_destroy(&pPath);
  }

  /*log_printf(pLogErr, "<--PATH_SET [%p]\n", (*ppAttr)->pASPathRef);*/
}

// -----[ bgp_attr_path_prepend ]------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 */
int bgp_attr_path_prepend(SBGPAttr ** ppAttr, uint16_t uAS, uint8_t uAmount)
{
  SBGPPath * pPath;

  /*log_printf(pLogErr, "-->PATH_PREPEND [%p]\n", (*ppAttr)->pASPathRef);*/

  // Create extern AS-Path copy
  if ((*ppAttr)->pASPathRef == NULL)
    pPath= path_create();
  else
    pPath= path_copy((*ppAttr)->pASPathRef);

  // Prepend
  while (uAmount-- > 0) {
    if (path_append(&pPath, uAS) < 0)
      return -1;
  }

  // Intern path
  bgp_attr_set_path(ppAttr, pPath);

  /*log_printf(pLogErr, "<--PATH_PREPEND [%p]\n", (*ppAttr)->pASPathRef);*/
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_comm_copy ]--------------------------------------
/**
 * Copy the Communities attribute and update the references into the
 * global path repository.
 *
 * Pre-condition:
 *   the source Communities must be intern, that is it must already be in
 *   the global Communities repository.
 */
static inline void _bgp_attr_comm_copy(SBGPAttr * pAttr,
				       SCommunities * pCommunities)
{
  pAttr->pCommunities= comm_hash_add(pCommunities);

  // Check that the referenced Communities is equal to the source
  // Communities (if not, that means that the source Communities was
  // not intern)
  assert(pAttr->pCommunities == pCommunities);
}

// -----[ _bgp_attr_comm_destroy ]-----------------------------------
/**
 * Destroy the Communities attribute and removes the global
 * reference.
 */
static inline void _bgp_attr_comm_destroy(SBGPAttr ** ppAttr)
{
  if ((*ppAttr)->pCommunities != NULL) {
    comm_hash_remove((*ppAttr)->pCommunities);
    (*ppAttr)->pCommunities= NULL;
  }
}

// -----[ bgp_attr_set_comm ]----------------------------------------
/**
 *
 */
void bgp_attr_set_comm(SBGPAttr ** ppAttr,
			      SCommunities * pCommunities)
{
  _bgp_attr_comm_destroy(ppAttr);
  if (pCommunities != NULL) {
    (*ppAttr)->pCommunities= comm_hash_add(pCommunities);
    assert((*ppAttr)->pCommunities != NULL);
    if ((*ppAttr)->pCommunities != pCommunities)
      comm_destroy(&pCommunities);
  }
}

// -----[ bgp_attr_comm_append ]-------------------------------------
/**
 *
 */
int bgp_attr_comm_append(SBGPAttr ** ppAttr, comm_t tCommunity)
{
  SCommunities * pCommunities;

  // Create extern Communities copy
  if ((*ppAttr)->pCommunities == NULL)
    pCommunities= comm_create();
  else
    pCommunities= comm_copy((*ppAttr)->pCommunities);

  // Add new community value
  if (comm_add(&pCommunities, tCommunity)) {
    comm_destroy(&pCommunities);
    return -1;
  }

  // Intern Communities
  bgp_attr_set_comm(ppAttr, pCommunities);
  return 0;
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

  // Create extern Communities copy
  pCommunities= comm_copy((*ppAttr)->pCommunities);

  // Remove given community (this will destroy the Communities if
  // needed)
  comm_remove(&pCommunities, tCommunity);

  // Intern Communities
  bgp_attr_set_comm(ppAttr, pCommunities);
}

// -----[ bgp_attr_comm_strip ]--------------------------------------
/**
 *
 */
void bgp_attr_comm_strip(SBGPAttr ** ppAttr)
{
  _bgp_attr_comm_destroy(ppAttr);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE EXTENDED-COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_ecomm_copy ]-------------------------------------
static inline void _bgp_attr_ecomm_copy(SBGPAttr * pAttr,
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
static inline void _bgp_attr_ecomm_destroy(SBGPAttr * pAttr)
{
  ecomm_destroy(&pAttr->pECommunities);
}

// -----[ bgp_attr_ecomm_append ]------------------------------------
/**
 *
 */
int bgp_attr_ecomm_append(SBGPAttr ** ppAttr, SECommunity * pComm)
{
  if ((*ppAttr)->pECommunities == NULL)
    (*ppAttr)->pECommunities= ecomm_create();
  return ecomm_add(&(*ppAttr)->pECommunities, pComm);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGINATOR-ID
//
/////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE CLUSTER-ID-LIST
//
/////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

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
    _bgp_attr_comm_destroy(ppAttr);
    _bgp_attr_ecomm_destroy(*ppAttr);

    /* Route-reflection */
    bgp_attr_originator_destroy(*ppAttr);
    bgp_attr_cluster_list_destroy(*ppAttr);
    FREE(*ppAttr);
  }
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

