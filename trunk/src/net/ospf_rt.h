// ==================================================================
// @(#)ospf_rt.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 14/05/2005
// ==================================================================

#ifndef __NET_OSPF_RT_H__
#define __NET_OSPF_RT_H__

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/memory.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/ospf.h>
#include <net/link.h>


#define OSPF_PATH_TYPE_INTRA       0
#define OSPF_PATH_TYPE_INTER       1
#define OSPF_PATH_TYPE_EXTERNAL_1  2
#define OSPF_PATH_TYPE_EXTERNAL_2  3

#define OSPF_NO_IP_NEXT_HOP 0

// ----- ospf_rt_test() -----------------------------------------------
extern int ospf_rt_test();

///////////////////////////////////////////////////////////////////////////
//////  OSPF NEXT HOP FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- ospf_next_hop_create -------------------------------------------------------
extern SOSPFNextHop * ospf_next_hop_create(SNetLink * pLink, net_addr_t tAddr);
// ----- ospf_next_hop_destroy -------------------------------------------------------
extern void ospf_next_hop_destroy(SOSPFNextHop ** ppNH);

// ----- ospf_next_hops_compare -----------------------------------------------
extern int ospf_next_hops_compare(void * pItem1, void * pItem2, unsigned int uEltSize);
// ----- ospf_nh_list_create --------------------------------------------------
extern next_hops_list_t * ospf_nh_list_create();
// ----- ospf_nh_list_destroy --------------------------------------------------
extern void ospf_nh_list_destroy(next_hops_list_t ** pNHList);
// ----- ospf_nh_list_add --------------------------------------------------
extern int ospf_nh_list_add(next_hops_list_t * pNHList, SOSPFNextHop * pNH);
// ----- ospf_nh_list_dump --------------------------------------------------
/*
   pcSpace should not be NULL! 
   USAGE pcSapace = "" or 
         pcSpace = "\t"
*/
extern void ospf_nh_list_dump(FILE * pStream, next_hops_list_t * pNHList, char * pcSpace, int inLine);

///////////////////////////////////////////////////////////////////////////
//////  ROUTING TABLE OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////

// ----- rt_perror -------------------------------------------------------
extern void OSPF_rt_perror(FILE * pStream, int iErrorCode);
// ----- route_info_create ------------------------------------------
extern SOSPFRouteInfo * OSPF_route_info_create(ospf_dest_type_t  tOSPFDestinationType,
                                       SPrefix            sPrefix,
				       uint32_t           uWeight,
				       ospf_area_t        tOSPFArea,
				       ospf_path_type_t   tOSPFPathType,
				       next_hops_list_t * pNHList);
				       
// ----- OSPF_route_info_add_nextHop -----------------------------------------
int OSPF_route_info_add_nextHop(SOSPFRouteInfo * pRouteInfo, SOSPFNextHop * pNH);
// ----- OSPF_route_info_dump ------------------------------------------------
extern void OSPF_route_info_dump(FILE * pStream, SOSPFRouteInfo * pRouteInfo);
// ----- OSPF_rt_find_exact --------------------------------------------------
extern SOSPFRouteInfo * OSPF_rt_find_exact(SOSPFRT * pRT, SPrefix sPrefix,
			      net_route_type_t tType);
// ----- OSPF_rt_dump -------------------------------------------------------------
void OSPF_rt_dump(FILE * pStream, SOSPFRT * pRT);

// ----- route_info_destroy --------------------------------------------------
extern void OSPF_route_info_destroy(SOSPFRouteInfo ** ppOSPFRouteInfo);


// ----- rt_create --------------------------------------------------
extern SOSPFRT * OSPF_rt_create();
// ----- rt_destroy -------------------------------------------------
extern void OSPF_rt_destroy(SOSPFRT ** ppRT);
// ----- rt_find_best -----------------------------------------------
extern SOSPFRouteInfo * OSPF_rt_find_best(SOSPFRT * pRT, net_addr_t tAddr,
				    net_route_type_t tType);
// ----- rt_find_exact ----------------------------------------------
extern SOSPFRouteInfo * OSPF_rt_find_exact(SOSPFRT * pRT, SPrefix sPrefix,
				     net_route_type_t tType);
// ----- rt_add_route -----------------------------------------------
extern int OSPF_rt_add_route(SOSPFRT * pRT, SPrefix sPrefix,
			SOSPFRouteInfo * pRouteInfo);
// ----- rt_del_route -----------------------------------------------
extern int OSPF_rt_del_route(SOSPFRT * pRT, SPrefix * pPrefix,
			SNetLink * pNextHopIf, net_route_type_t tType);


// ----- rt_for_each -------------------------------------------------------
extern int OSPF_rt_for_each(SNetRT * pRT, FRadixTreeForEach fForEach, void * pContext);

#endif

