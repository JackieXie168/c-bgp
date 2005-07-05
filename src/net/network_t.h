// ==================================================================
// @(#)network_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 17/05/2005
// ==================================================================

#ifndef __NET_NETWORK_T_H__
#define __NET_NETWORK_T_H__

#include <libgds/types.h>
#include <net/net_types.h>
#include <net/protocol.h>
#include <net/domain_t.h>

extern const net_addr_t MAX_ADDR;

#define OSPF_SUPPORT

typedef struct {
#ifdef __EXPERIMENTAL__
  STrie * pNodes;
#else
  SRadixTree * pNodes;
#endif
  SPtrArray  * pDomains;
} SNetwork;

typedef struct {
  net_addr_t tAddr;
  SPtrArray * aInterfaces;
  char * pcName;
  uint32_t  uAS;
  
  SNetwork * pNetwork;
  SPtrArray * pLinks;
  SNetRT * pRT;
#ifdef OSPF_SUPPORT
  SUInt32Array * pOSPFAreas; 
  SOSPFRT * pOspfRT; //OSPF routing table
  SUInt16Array * pIGPDomains; //TODO define type for list of domains numbers
#endif
  SNetProtocols * pProtocols;
  SNetDomain * pDomain;
} SNetNode;

// ----- node_links_compare -----------------------------------------
int node_links_compare(void * pItem1, void * pItem2,
		       unsigned int uEltSize);

#endif

