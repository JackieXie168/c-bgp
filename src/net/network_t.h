// ==================================================================
// @(#)network_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 4/07/2003
// @lastdate 05/08/2005
// ==================================================================

#ifndef __NET_NETWORK_T_H__
#define __NET_NETWORK_T_H__

#include <libgds/types.h>
#include <libgds/patricia-tree.h>
#include <net/net_types.h>
#include <net/protocol.h>
#include <net/domain_t.h>

extern const net_addr_t MAX_ADDR;

typedef struct {
  STrie * pNodes;
  SPtrArray  * pSubnets; //Subnets are stored here only to have a single point to access 
                         //to destroy them 
} SNetwork;

#endif

