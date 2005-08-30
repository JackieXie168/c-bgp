// ==================================================================
// @(#)ospf.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 14/05/2005
// ==================================================================

#ifndef __NET_OSPF_H__
#define __NET_OSPF_H__

#include <net/net_types.h>
#include <net/subnet.h>
#include <net/link.h>

#define OSPF_NO_AREA	      0xffffffff 
#define BACKBONE_AREA 0

//First is only a warning
#define OSPF_LINK_OK                        0
#define OSPF_LINK_TO_MYSELF_NOT_IN_AREA    -1 
#define OSPF_SOURCE_NODE_NOT_IN_AREA       -2
#define OSPF_DEST_NODE_NOT_IN_AREA         -3
#define OSPF_DEST_SUBNET_NOT_IN_AREA       -4
#define OSPF_SOURCE_NODE_LINK_MISSING      -5

/*
   Assumption: pSubNet is valid only for destination type Subnet.
   The filed can be used only if pPrefix has a 32 bit netmask.
*/
typedef struct {
  SNetLink * pLink;  //link towards next-hop
  net_addr_t tAddr;  //ip address of next hop
  //TODO add advertising router
} SOSPFNextHop;

// ----- ospf test function --------------------------------------------
extern int ospf_test();

///////////////////////////////////////////////////////////////////////////
//////  NODE OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- node_add_OSPFArea --------------------------------------------
extern int node_add_OSPFArea(SNetNode * pNode, uint32_t OSPFArea);
// ----- node_is_BorderRouter --------------------------------------------
extern int node_is_BorderRouter(SNetNode * pNode);
// ----- node_is_InternalRouter --------------------------------------------
extern int node_is_InternalRouter(SNetNode * pNode);
// ----- node_ospf_rt_add_route --------------------------------------------
extern int node_ospf_rt_add_route(SNetNode     * pNode,     ospf_dest_type_t  tOSPFDestinationType,
                       SPrefix        sPrefix,   net_link_delay_t        uWeight,
		       ospf_area_t    tOSPFArea, ospf_path_type_t  tOSPFPathType,
		       next_hops_list_t * pNHList);
// ----- node_belongs_to_area -----------------------------------------------
extern int node_belongs_to_area(SNetNode * pNode, uint32_t tArea);
// ----- node_link_set_ospf_area ------------------------------------------
extern int node_link_set_ospf_area(SNetNode * pNode, SPrefix sPfxPeer, ospf_area_t tArea);
///////////////////////////////////////////////////////////////////////////
//////  SUBNET OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- subnet_OSPFArea -------------------------------------------------
extern int subnet_set_OSPFArea(SNetSubnet * pSubnet, uint32_t uOSPFArea);
// ----- subnet_getOSPFArea ----------------------------------------------
extern uint32_t subnet_get_OSPFArea(SNetSubnet * pSubnet);
// ----- subnet_belongs_to_area ---------------------------------------------
int subnet_belongs_to_area(SNetSubnet * pSubnet, uint32_t tArea);

///////////////////////////////////////////////////////////////////////////
//////  OSPF ROUTING TABLE FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- ospf_node_rt_dump ------------------------------------------------------------------
extern void ospf_node_rt_dump(FILE * pStream, SNetNode * pNode, int iOption);
///////////////////////////////////////////////////////////////////////////
//////  OSPF DOMAIN TABLE FUNCTION
///////////////////////////////////////////////////////////////////////////
//----- ospf_domain_build_route ---------------------------------------------------------------
extern int ospf_domain_build_route(uint16_t uOSPFDomain);
#endif

