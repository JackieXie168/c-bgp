// ==================================================================
// @(#)domain.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 16/09/2004
// @lastdate 16/09/2004
// ==================================================================

#ifndef __NET_DOMAIN_H__
#define __NET_DOMAIN_H__

#include <net/prefix.h>
#include <net/network.h>
#include <net/domain_t.h>



// ----- domain_create -----------------------------------------------
SNetDomain * domain_create(uint32_t uAS, char * pcName);
// ----- domain_destoy -----------------------------------------------
void domain_destroy(SNetDomain ** pDomain);
// ----- domain_node_add ---------------------------------------------
int domain_node_add(SNetDomain * pDomain, SNetNode * pNode);
// ----- domain_get_node ---------------------------------------------
SNetNode * domain_get_node(SNetDomain * pDomain, net_addr_t tAddr);
// ----- domain_get_as -----------------------------------------------
uint32_t domain_get_as(SNetDomain * pDomain);
// ----- domain_get_name ---------------------------------------------
char * domain_get_name(SNetDomain * pDomain);

#endif
