// ==================================================================
// @(#)domain.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 16/09/2004
// @lastdate 14/02/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>

#include <libgds/memory.h>
#include <net/domain.h>

// ----- net_domain_create ------------------------------------------
/**
 *
 *
 */
SNetDomain * domain_create(uint32_t uAS, char * pcName)
{
  SNetDomain * pDomain = MALLOC(sizeof(SNetDomain));

  pDomain->uAS = uAS;
  pDomain->pcName = MALLOC(strlen(pcName)+1);
  memcpy(pDomain->pcName, pcName, strlen(pcName)+1);
  pDomain->rtNodes = radix_tree_create(sizeof(net_addr_t)*4, NULL);
  return pDomain;
}

// ----- domain_destroy ----------------------------------------------
/**
 *
 *
 */
void domain_destroy(SNetDomain ** pDomain)
{
  if (*pDomain != NULL) { 
    FREE((*pDomain)->pcName);
    radix_tree_destroy(&(*pDomain)->rtNodes);
    FREE(*pDomain);
    *pDomain=NULL;
  }
}

// ----- domain_get_as -----------------------------------------------
/**
 *
 *
 */
uint32_t domain_get_as (SNetDomain * pDomain)
{
  return pDomain->uAS;
}

// ----- domain_get_name ---------------------------------------------
/**
 *
 *
 */
char * domain_get_name(SNetDomain * pDomain)
{
  return pDomain->pcName;
}

// ----- domain_node_add ---------------------------------------------
/**
 *
 *
 */
int domain_node_add(SNetDomain * pDomain, SNetNode * pNode)
{
  return radix_tree_add(pDomain->rtNodes, pNode->tAddr, sizeof(net_addr_t)*4, pNode);
}

// ----- domain_node_get ---------------------------------------------
/**
 *
 *
 */
SNetNode * domain_node_get(SNetDomain * pDomain, net_addr_t tAddr)
{
  return radix_tree_get_exact(pDomain->rtNodes, tAddr, sizeof(net_addr_t)*4);
}
