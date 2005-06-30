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

#define NO_AREA	      0xffffffff //can be a problem! 
#define BACKBONE_AREA 0

#define OSPF_DESTINATION_TYPE_NETWORK 0
#define OSPF_DESTINATION_TYPE_ROUTER  1





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
                       SPrefix        sPrefix,   uint32_t          uWeight,
		       ospf_area_t    tOSPFArea, ospf_path_type_t  tOSPFPathType,
		       next_hops_list_t * pNHList);
///////////////////////////////////////////////////////////////////////////
//////  SUBNET OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- subnet_OSPFArea -------------------------------------------------
extern void subnet_set_OSPFArea(SNetSubnet * pSubnet, uint32_t uOSPFArea);
// ----- subnet_getOSPFArea ----------------------------------------------
extern uint32_t subnet_get_OSPFArea(SNetSubnet * pSubnet);


#endif

