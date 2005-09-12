// ==================================================================
// @(#)spt_vertex.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 4 July 2005
// @lastdate 4 July 2005
// ==================================================================

#ifndef __NET_SPT_VERTEX_H__
#define __NET_SPT_VERTEX_H__

#include <assert.h>
#include <libgds/array.h>
#include <libgds/memory.h>
#include <net/net_types.h>
#include <net/network_t.h>
#include <net/link.h>
#include <net/ospf_rt.h>
#include <net/network.h>

#define VINFO_LINKED_TO_SUBNET 0x01

#define spt_vertex_is_router(V) (V)->uDestinationType == NET_LINK_TYPE_ROUTER
#define spt_vertex_is_subnet(V) (V)->uDestinationType == NET_LINK_TYPE_STUB || (V)->uDestinationType == NET_LINK_TYPE_TRANSIT
#define spt_vertex_to_router(V) ((SNetNode *)((V)->pObject))
#define spt_vertex_to_subnet(V) ((SNetSubnet *)((V)->pObject))
#define spt_vertex_is_linked_to_subnet(V) ((V)->tVertexInfo & VINFO_LINKED_TO_SUBNET)
#define spt_vertex_linked_to_subnet(V) ((V)->tVertexInfo = (V)->tVertexInfo | VINFO_LINKED_TO_SUBNET)
typedef SPtrArray spt_vertex_list_t;

 typedef struct vertex_t {
   uint8_t              uDestinationType;    /* TNET_LINK_TYPE_ROUTER , 
                                                NET_LINK_TYPE_TRANSIT, NET_LINK_TYPE_STUB */ 
   void               * pObject;
   next_hops_list_t   * pNextHops;
   net_link_delay_t     uIGPweight;  
   SPtrArray          * aSubnets;   //to optimize routing table computation without re-search for
                                    //subnet
   uint8_t 	        tVertexInfo;
   SPtrArray          * fathers;         //ONLY FOR DEBUG we can remove
   SPtrArray          * sons;            //ONLY FOR DEBUG we can remove
 } SSptVertex;


// ----- spt_vertex_create -----------------------------------------------------------------
SSptVertex * spt_vertex_create(SNetwork * pNetwork, SNetLink * pLink, SSptVertex * pVFather); 
// ----- spt_vertex_create_byRouter ---------------------------------------------------
SSptVertex * spt_vertex_create_byRouter(SNetNode * pNode, net_link_delay_t uIGPweight);
// ----- spt_vertex_create_bySubnet ---------------------------------------
SSptVertex * spt_vertex_create_bySubnet(SNetSubnet * pSubnet, net_link_delay_t uIGPweight);
 // ----- spt_vertex_compare -----------------------------------------------
int spt_vertex_compare(void * pItem1, void * pItem2, unsigned int uEltSize);
// ----- spt_vertex_get_links ---------------------------------------
SPtrArray * spt_vertex_get_links(SSptVertex * pVertex);
// ----- spt_vertex_get_id ---------------------------------------
SPrefix spt_vertex_get_id(SSptVertex * pVertex);
// ----- spt_vertex_add_subnet ---------------------------------------------------
int spt_vertex_add_subnet(SSptVertex * pCurrentVertex, SNetLink * pCurrentLink);

// ----- spt_vertex_belongs_to_area ---------------------------------------------------
int spt_vertex_belongs_to_area(SSptVertex * pVertex, ospf_area_t tArea);

// ----- calculate_next_hop -----------------------------------------------------------
void spt_calculate_next_hop(SSptVertex * pRoot, SSptVertex * pParent, 
                                      SSptVertex * pDestination, SNetLink * pLink);
// ----- spt_vertex_destroy ---------------------------------------
void spt_vertex_destroy(SSptVertex ** ppVertex);
// ----- spt_get_best_candidate -----------------------------------------------
SSptVertex * spt_get_best_candidate(SPtrArray * paGrayVertexes);

// ----- spt_vertex_dst ---------------------------------------
void spt_vertex_dst(void ** ppItem);

// ----- node_ospf_compute_spt ---------------------------------------------------
SRadixTree * node_ospf_compute_spt(SNetNode * pNode, uint16_t IGPDomainNumber, ospf_area_t tArea);
			      
// ----- spt_dump_dot ------------------------------------------------------------
void spt_dump_dot(FILE * pStream, SRadixTree * pSpt, net_addr_t tRadixAddr);
#endif

