/* 
 * File:   routing_state.h
 * Author: yo
 *
 * Created on 24 mars 2011, 11:04
 */

#ifndef ROUTING_STATE_H
#define	ROUTING_STATE_H



#include <tracer/state.h>

#include <tracer/transition.h>
#include <tracer/graph.h>
#include <tracer/tracer.h>
#include <tracer/state.h>



typedef struct loc_rib_States_t;


typedef struct bgp_session_info_t{
   net_addr_t            neighbor_addr;
   unsigned int          send_seq_num;
   unsigned int          recv_seq_num;
   bgp_route_t **        adj_rib_IN_routes;
   bgp_route_t **        adj_rib_OUT_routes;
   unsigned int          nb_adj_rib_in_routes;
   unsigned int          nb_adj_rib_out_routes;

   net_addr_t            next_hop;
   /** Optional source IP address. */
   net_addr_t            src_addr;
   int                   last_error;


   //bgp_rib_t           * adj_rib[RIB_MAX];

}bgp_session_info_t;

typedef struct bgp_sessions_info_t{
   unsigned int          nb_bgp_session_info_;
   bgp_session_info_t ** bgp_session_info;
}bgp_sessions_info_t;

typedef struct local_rib_info_t{
   unsigned int          nb_local_rib_elem;
   bgp_route_t ** bgp_route_;
}local_rib_info_t;

typedef struct routing_info_t{
   net_rt_t            *  node_rt_t;
   local_rib_info_t    *  bgp_router_loc_rib_t;
   //bgp_peers_t         * bgp_router_peers;
   bgp_sessions_info_t * bgp_sessions_info;
}routing_info_t;

typedef struct couple_node_routinginfo_t{
    net_node_t * node;
    routing_info_t * routing_info;
} couple_node_routinginfo_t;


// TODO : brol
typedef struct routing_state_t{
  struct state_t              * state;
  couple_node_routinginfo_t **    couples_node_routing_info;

  struct loc_rib_States_t *     the_common_loc_rib;
    // les rib   (des couples de [node,rib]   )
    // adj-rib-in and -out pour chaque voisin de chaque noeud.
    // pour chaque noeud :  - rib
    //                      - pour chaque voisin :
    //                                      -adj-rib-in
    //                                      -adj-rib-out

}  routing_state_t;


typedef struct nodes_locrib_t {
     net_node_t          * node;
     unsigned int          nb_local_rib_elem;
     bgp_route_t        ** bgp_route_;
}nodes_locrib_t;


typedef struct loc_rib_States_t{    
    nodes_locrib_t      ** nodes_locribs;
    routing_state_t     ** routing_states;
    unsigned int        nb_routing_states;
    unsigned int        id;
    // ajouter lien vers l'image commune 
    // ou mettre un id (position dans le summary)
    // pour qu'on puisse identifier l'image qui correspond à cette state's locrib
    char                * filename;
    int                   exported;  // TRUE or FALSE
    
}loc_rib_States_t;


typedef struct local_rib_information_summary_t{
  loc_rib_States_t **    list_of_loc_rib_by_states;
  unsigned int              nb_loc_rib_by_states;
}local_rib_information_summary_t ;
  




#ifdef	__cplusplus
extern "C" {
#endif


routing_state_t * _routing_state_create(struct state_t * state);
routing_state_t *_routing_state_create_from_ST_TR(struct state_t * state, routing_state_t * from_routing_state, net_addr_t node_receiving_msg);

void _routing_state_flat_dump(gds_stream_t * stream,  routing_state_t * routing_state);

void _routing_state_dump(gds_stream_t * stream,  routing_state_t * routing_state);

int _routing_state_inject(routing_state_t * routing_state);

    

// 0 = identical
// other value, otherwise
int _routing_states_equivalent(routing_state_t * rs1, routing_state_t * rs2 );

void routing_state_export_dot_OLD(gds_stream_t * stream, routing_state_t * rs);

void routing_state_export_loc_rib_dot(routing_state_t * rs);

#ifdef	__cplusplus
}
#endif

#endif	/* ROUTING_STATE_H */

