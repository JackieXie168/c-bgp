

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>

#include <libgds/memory.h>
#include <libgds/stack.h>

#include <assert.h>
#include <libgds/str_util.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#include <bgp/peer.h>


#include <tracer/state.h>
#include <tracer/tracer.h>
#include <tracer/graph.h>

#include <sim/tunable_scheduler.h>

#include <libgds/fifo_tunable.h>


#include <libgds/trie.h>
#include <../net/net_types.h>
#include <bgp/types.h>


#include "state.h"
#include "transition.h"
#include "routing_state.h"








bgp_session_info_t * _one_session_info_create(bgp_peer_t * peer )
{

    bgp_session_info_t * thesessioninfo = (bgp_session_info_t *) MALLOC( sizeof(bgp_session_info_t) );
    bgp_route_t ** bgp_route_array;
    unsigned int index;

    thesessioninfo->recv_seq_num=peer->recv_seq_num;
    thesessioninfo->send_seq_num=peer->send_seq_num;
    thesessioninfo->neighbor_addr=peer->addr;
    thesessioninfo->last_error=peer->last_error;
    thesessioninfo->next_hop=peer->next_hop;
    thesessioninfo->src_addr=peer->src_addr;
    
    thesessioninfo->nb_adj_rib_in_routes = trie_num_nodes(peer->adj_rib[RIB_IN],1);
    bgp_route_array = (bgp_route_t **) (_trie_get_array(peer->adj_rib[RIB_IN])->data) ;
   // if (bgp_route_array == NULL)
   //     printf("il y a %d élément, mais get array   est NULL !  \n",trie_num_nodes(peer->adj_rib[RIB_IN],1));
    thesessioninfo->adj_rib_IN_routes =  (bgp_route_t **)
            MALLOC( thesessioninfo->nb_adj_rib_in_routes * sizeof(bgp_route_t *) );
    for (index= 0; index < thesessioninfo->nb_adj_rib_in_routes; index++) {
        thesessioninfo->adj_rib_IN_routes[index] = route_copy(bgp_route_array[index]);
    }

    thesessioninfo->nb_adj_rib_out_routes = trie_num_nodes(peer->adj_rib[RIB_OUT],1);
    bgp_route_array = (bgp_route_t **) (_trie_get_array(peer->adj_rib[RIB_OUT])->data) ;
    //if (bgp_route_array == NULL)
    //    printf("il y a %d élément, mais get array   est NULL !  \n",trie_num_nodes(peer->adj_rib[RIB_OUT],1));
    thesessioninfo->adj_rib_OUT_routes =  (bgp_route_t **)
            MALLOC( thesessioninfo->nb_adj_rib_out_routes * sizeof(bgp_route_t *) );
    for (index= 0; index < thesessioninfo->nb_adj_rib_out_routes; index++) {
        thesessioninfo->adj_rib_OUT_routes[index] = route_copy(bgp_route_array[index]);
    }
    return thesessioninfo;
}

bgp_sessions_info_t * _bgp_sessions_info_create(net_node_t * node)
{
    bgp_sessions_info_t * sessions_info = (bgp_sessions_info_t *) MALLOC(sizeof(bgp_sessions_info_t) );

    //pour chaque peer, creer le bgp_session_info
    bgp_peer_t * peer;
    unsigned int index;
    net_protocol_t * protocol;
    bgp_router_t * router;

    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol == NULL)
    {
        printf("ouille ouille ouille ..., ce noeud n'est pas un bgp router");
        return NULL;
    }
    router = (bgp_router_t *) protocol->handler;

    sessions_info->bgp_session_info = (bgp_session_info_t **) MALLOC( bgp_peers_size( router->peers) * sizeof(bgp_session_info_t *) );
    sessions_info->nb_bgp_session_info_ = bgp_peers_size( router->peers);

    for (index= 0; index < bgp_peers_size( router->peers); index++) {
        peer= bgp_peers_at(router->peers, index);
        sessions_info->bgp_session_info[index] = _one_session_info_create(peer);
    }
    return  sessions_info;
}

static void _bgp_one_session_info_dump(gds_stream_t * stream, bgp_session_info_t * bgp_session_info)
{
    unsigned int index;
   stream_printf(stream, "\t\t\tpeer : " );
         ip_address_dump(stream, bgp_session_info->neighbor_addr);
         stream_printf(stream, "\n" );
   if(get_state_type_of_dump()!=DUMP_ONLY_CONTENT)
		stream_printf(stream, "\t\t\t\ttcp send/recv seq num : %d/%d\n",bgp_session_info->send_seq_num , bgp_session_info->recv_seq_num );
   stream_printf(stream, "\t\t\t\tAdj-Rib IN : \n");
   for (index= 0; index < bgp_session_info->nb_adj_rib_in_routes; index++) {
       stream_printf(stream, "\t\t\t\t\t");
       route_dump(stream, bgp_session_info->adj_rib_IN_routes[index]);
       stream_printf(stream, "\n");
   }
   stream_printf(stream, "\t\t\t\tAdj-Rib OUT : \n");
   for (index= 0; index < bgp_session_info->nb_adj_rib_out_routes; index++) {
       stream_printf(stream, "\t\t\t\t\t");
       route_dump(stream, bgp_session_info->adj_rib_OUT_routes[index]);
       stream_printf(stream, "\n");
   }

}


static void _bgp_one_session_info_flat_dump(gds_stream_t * stream, net_node_t * node,  bgp_session_info_t * bgp_session_info)
{
    unsigned int index;
   for (index= 0; index < bgp_session_info->nb_adj_rib_in_routes; index++) {
    	node_dump_id(stream,node);
		stream_printf(stream, " IN from ");
        ip_address_dump(stream, bgp_session_info->neighbor_addr);
		stream_printf(stream, "   ");
        route_dump(stream, bgp_session_info->adj_rib_IN_routes[index]);
       stream_printf(stream, "\n");
   }

   for (index= 0; index < bgp_session_info->nb_adj_rib_out_routes; index++) {
    	node_dump_id(stream,node);
		stream_printf(stream, " OUT to ");
        ip_address_dump(stream, bgp_session_info->neighbor_addr);
		stream_printf(stream, "   ");
       route_dump(stream, bgp_session_info->adj_rib_OUT_routes[index]);
       stream_printf(stream, "\n");
   }

}


static void _bgp_sessions_info_dump(gds_stream_t * stream, bgp_sessions_info_t * sessions_info)
{
     stream_printf(stream, "\t\t\tBGP sessions \n");
    //pour chaque session, afficher les rib in et out :
    unsigned int index;
    for (index= 0; index < sessions_info->nb_bgp_session_info_; index++) {
        _bgp_one_session_info_dump(stream,sessions_info->bgp_session_info[index]);
    }
}
static void _bgp_sessions_info_flat_dump(gds_stream_t * stream, net_node_t * node, bgp_sessions_info_t * sessions_info)
{
    //pour chaque session, afficher les rib in et out :
    unsigned int index;
    for (index= 0; index < sessions_info->nb_bgp_session_info_; index++) {
        _bgp_one_session_info_flat_dump(stream,node, sessions_info->bgp_session_info[index]);
    }
}

static void _loc_rib_info_dump(gds_stream_t * stream, local_rib_info_t * info )
{
    unsigned int i;
    stream_printf(stream, "\t\t\tlocal RIB : \n");
    for ( i = 0 ; i<  info->nb_local_rib_elem ; i++)
    {
       stream_printf(stream, "\t\t\t\t");
        route_dump(stream, info->bgp_route_[i]);
       stream_printf(stream, "\n");
    }
}
static void _loc_rib_info_flat_dump(gds_stream_t * stream, net_node_t * node, local_rib_info_t * info )
{
    unsigned int i;
    for ( i = 0 ; i<  info->nb_local_rib_elem ; i++)
    {
    	node_dump_id(stream,node);
        stream_printf(stream, " loc-rib ");
        route_dump(stream, info->bgp_route_[i]);
        stream_printf(stream, "\n");
    }
}

local_rib_info_t * _routing_local_rib_create(net_node_t * node)
{
    unsigned int index;
    net_protocol_t * protocol;
    bgp_router_t * router;
    local_rib_info_t * loc_rib_info = (local_rib_info_t *) MALLOC(sizeof(local_rib_info_t) );
    bgp_route_t ** bgp_route_array ;

    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol == NULL)
    {
        printf("ouille ouille ouille ..., ce noeud n'est pas un bgp router");
        return NULL;
    }
    router = (bgp_router_t *) protocol->handler;

    loc_rib_info->nb_local_rib_elem = trie_num_nodes(router->loc_rib,1);

    bgp_route_array = (bgp_route_t **) (_trie_get_array(router->loc_rib)->data) ;
    loc_rib_info->bgp_route_ =  (bgp_route_t **)
            MALLOC(loc_rib_info->nb_local_rib_elem * sizeof(bgp_route_t *) );
    for (index= 0; index < loc_rib_info->nb_local_rib_elem; index++) {
        loc_rib_info->bgp_route_[index] = route_copy(bgp_route_array[index]);
    }
    return loc_rib_info;
}

/*
_routing_node_rt_create(net_node_t * node)
{


}*/

routing_info_t * _routing_info_create(net_node_t * node)
{
  routing_info_t * info = (routing_info_t *) MALLOC( sizeof(routing_info_t) );


  info->node_rt_t = rt_deep_copy(node->rt);

  info->bgp_router_loc_rib_t = _routing_local_rib_create(node);

  //info->bgp_router_peers = ;

  info->bgp_sessions_info = _bgp_sessions_info_create(node);

  return info;
}

static void _routing_info_dump(gds_stream_t * stream, routing_info_t * info )
{
     //stream_printf(stream, " Node : %d" , coupleNR->node->rid );
     _loc_rib_info_dump(stream,info->bgp_router_loc_rib_t );
     _bgp_sessions_info_dump(stream,info->bgp_sessions_info);
}
static void _routing_info_flat_dump(gds_stream_t * stream, net_node_t * node, routing_info_t * info )
{
     //stream_printf(stream, " Node : %d" , coupleNR->node->rid );
     _loc_rib_info_flat_dump(stream,node, info->bgp_router_loc_rib_t );
     _bgp_sessions_info_flat_dump(stream,node,info->bgp_sessions_info);

}


couple_node_routinginfo_t * _couple_node_routinginfo_create(net_node_t * node)
{
 couple_node_routinginfo_t * couple;
 couple = (couple_node_routinginfo_t *) MALLOC(sizeof(couple_node_routinginfo_t));

 couple->node=node;
 couple->routing_info = _routing_info_create(node);

 return couple;

}

// ----- state_create ------------------------------------------------
routing_state_t * _routing_state_create(state_t * state)
{
  routing_state_t * routing_state;
  routing_state= (routing_state_t *) MALLOC(sizeof(routing_state_t));

  routing_state->state = state;
  routing_state->couples_node_routing_info = (couple_node_routinginfo_t **) MALLOC(state->graph->tracer->nb_nodes * sizeof(couple_node_routinginfo_t *));

  unsigned int i = 0;
  for(i=0 ; i< state->graph->tracer->nb_nodes ; i++)
  {
      routing_state->couples_node_routing_info[i] = _couple_node_routinginfo_create(state->graph->tracer->nodes[i]);
  }

  return routing_state;
}

static void _couple_node_routinginfo_dump(gds_stream_t * stream, couple_node_routinginfo_t * coupleNR)
{
    stream_printf(stream, "\t\tNode : " );
    node_dump_id(stream,coupleNR->node);
    stream_printf(stream, "\n" );

    _routing_info_dump(stream,coupleNR->routing_info);
}

static void _couple_node_routinginfo_flat_dump(gds_stream_t * stream, couple_node_routinginfo_t * coupleNR)
{
    //node_dump_id(stream,coupleNR->node);
    _routing_info_flat_dump(stream,coupleNR->node,coupleNR->routing_info);
}

 void _routing_state_flat_dump(gds_stream_t * stream, routing_state_t * routing_state)
{
//  stream_printf(stream, "\tRouting State : \n");
  unsigned int i = 0;
  for(i=0 ; i< routing_state->state->graph->tracer->nb_nodes ; i++)
  {
      _couple_node_routinginfo_flat_dump(stream,routing_state->couples_node_routing_info[i]);
  }
}

 void _routing_state_dump(gds_stream_t * stream, routing_state_t * routing_state)
{
  stream_printf(stream, "\tRouting State : \n");
  unsigned int i = 0;
  for(i=0 ; i< routing_state->state->graph->tracer->nb_nodes ; i++)
  {
      _couple_node_routinginfo_dump(stream,routing_state->couples_node_routing_info[i]);
  }
}

static int bgp_router_inject_bgp_session_information(bgp_peer_t * peer, bgp_session_info_t * info)
{
    unsigned int i;

    peer->send_seq_num = info->send_seq_num;
    peer->recv_seq_num = info->recv_seq_num;
    
    if(peer->addr != info->neighbor_addr) printf("peer->addr UTILE !!\n");
    peer->addr = info->neighbor_addr;
    if(peer->last_error != info->last_error) printf("peer>last_error UTILE !!\n");
    peer->last_error=info->last_error;
    if(peer->next_hop != info->next_hop) printf("peer->next_hop UTILE !!\n");
    peer->next_hop=info->next_hop;
    if(peer->src_addr != info->src_addr) printf("peer->src_addr UTILE !!\n");
    peer->src_addr=info->src_addr;


    rib_destroy(&(peer->adj_rib[RIB_IN]));
    peer->adj_rib[RIB_IN] = rib_create(0);

    for(i=0 ; i < info->nb_adj_rib_in_routes; i++)
    {
        // ajouter une copie de la route que le tracer mémorise !
        //printf("ATTENTION, placer une copie de la route ");
        rib_add_route(peer->adj_rib[RIB_IN], route_copy(info->adj_rib_IN_routes[i]));
    }

    rib_destroy(&(peer->adj_rib[RIB_OUT]));
    peer->adj_rib[RIB_OUT] = rib_create(0);

    for(i=0 ; i < info->nb_adj_rib_out_routes; i++)
    {
        // ajouter une copie de la route que le tracer mémorise !
        //printf("ATTENTION, placer une copie de la route ");
        rib_add_route(peer->adj_rib[RIB_OUT], route_copy(info->adj_rib_OUT_routes[i]));
    }

    return 1;
}

static int bgp_router_inject_loc_rib_info(bgp_router_t * router, local_rib_info_t * loc_rib_info)
{
        unsigned int i;
        rib_destroy(&(router->loc_rib));
        router->loc_rib = rib_create(0);

        for(i=0 ; i < loc_rib_info->nb_local_rib_elem; i++)
        {
            rib_add_route(router->loc_rib, route_copy(loc_rib_info->bgp_route_[i]));
        }
        return i;
}

static int _node_inject_routing_info(net_node_t * node, routing_info_t * routing_info )
{
    // inject bgp sessions
    bgp_sessions_info_t * sessions_info = routing_info->bgp_sessions_info;
    local_rib_info_t * loc_rib_info = routing_info->bgp_router_loc_rib_t;

    bgp_peer_t * peer;
    unsigned int index;
    net_protocol_t * protocol;
    bgp_router_t * router;

    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol == NULL)
    {
        printf("ouille ouille ouille ..., ce noeud n'est pas un bgp router");
        return -3;
    }
    router = (bgp_router_t *) protocol->handler;

    //pour chaque peer, donner les infos de session bgp

    if(bgp_peers_size( router->peers) != sessions_info->nb_bgp_session_info_)
    {
        printf("ATTENTION !!! pas autant de peer dans le noeud de cbgp que de session qu'on a mémorisé plus tôt pour ce même noeud!!\n");
        return -1;
    }

    for (index= 0; index < bgp_peers_size( router->peers); index++) {
        peer= bgp_peers_at(router->peers, index);
        if(peer->addr !=  sessions_info->bgp_session_info[index]->neighbor_addr)
        {
            printf("ATTENTION !!! les peer ne sont pas dans le même ordre qu'avant !?\n");
            return -2;
        }
        bgp_router_inject_bgp_session_information(peer,sessions_info->bgp_session_info[index]);
    }

    // local_rib
    bgp_router_inject_loc_rib_info(router,loc_rib_info);


    // node_rt_t
// TODO compare before copying !  in order to see if it is accurate.
    //
    rt_destroy(&(node->rt));
    node->rt = rt_deep_copy(routing_info->node_rt_t );


    return index;
}

 int _routing_state_inject(routing_state_t * routing_state)
{
  unsigned int i = 0;

if(STATE_DEBBUG==1){
  int level=1;     int ilevel; for(ilevel = 0 ; ilevel < level ; ilevel++) printf("  ");
  printf("Routing state inject\n");
}

  for(i=0 ; i< routing_state->state->graph->tracer->nb_nodes ; i++)
  {
      if(STATE_DEBBUG==1)
		{
	int level=2;  int ilevel;    for( ilevel = 0 ; ilevel < level ; ilevel++) printf("  ");
     printf("node ");
      node_dump_id(gdsout,routing_state->couples_node_routing_info[i]->node );
      printf("\n");
}
      _node_inject_routing_info(routing_state->couples_node_routing_info[i]->node ,
                                    routing_state->couples_node_routing_info[i]->routing_info);
  }
  return i;
}

// 0 --> identical
//other valuer -->otherwize
int _one_bgp_session_compare( bgp_session_info_t * bgpi1, bgp_session_info_t * bgpi2)
{
    unsigned int index;
    if(bgpi1->nb_adj_rib_in_routes != bgpi2->nb_adj_rib_in_routes
            || bgpi1->nb_adj_rib_out_routes != bgpi2->nb_adj_rib_out_routes )
        return -710;

    //adj-rib-in
    for (index= 0; index < bgpi1->nb_adj_rib_in_routes; index++) {
       if( 1 != route_equals(bgpi1->adj_rib_IN_routes[index],bgpi2->adj_rib_IN_routes[index]))
       {
           return -720;
       }
   }

        //adj-rib-out
    for (index= 0; index < bgpi1->nb_adj_rib_out_routes; index++) {
       if( 1 != route_equals(bgpi1->adj_rib_OUT_routes[index],bgpi2->adj_rib_OUT_routes[index]))
       {
           return -730;
       }
   }
    return 0;

}

//0=identical
// other value  --> otherwise
int _bgp_sessions_compare(bgp_sessions_info_t * bgp_i_1, bgp_sessions_info_t * bgp_i_2  )
{
   unsigned int i;

   if(bgp_i_1->nb_bgp_session_info_ != bgp_i_2->nb_bgp_session_info_)
       return -660;

   for(i = 0; i < bgp_i_1->nb_bgp_session_info_ ; i++)
   {
       if ( 0 != _one_bgp_session_compare(bgp_i_1->bgp_session_info[i],bgp_i_2->bgp_session_info[i]  ) )
           return -670;
   }
   return  0;
}

// 0 = identical
// other value, otherwise
int _rib_compare(local_rib_info_t * rib1, local_rib_info_t * rib2)
{
// 1rst version :
//  have to have the same route in the same order !

    unsigned int i;

    if(rib1->nb_local_rib_elem != rib2->nb_local_rib_elem)
        return -700;

    for(i = 0; i < rib1->nb_local_rib_elem ; i++)
    {
        if ( route_equals(rib1->bgp_route_[i], rib2->bgp_route_[i] ) !=1 )
            return -800;
    }
    return 0;
}

// 0 = identical
// other value, otherwise
int _compare_rt_entries(rt_entries_t * entries1, rt_entries_t * entries2)
{
    if (ptr_array_length(entries1) != ptr_array_length(entries2))
        return -678;

    unsigned int taille = ptr_array_length(entries1);

    unsigned int i;
    for(i=0; i<taille; i++)
    {
       rt_entry_t * entry1 = (rt_entry_t *) entries1->data[i];
       rt_entry_t * entry2 = (rt_entry_t *) entries2->data[i];
       if(0 != rt_entry_compare(entry1, entry2))
           return -679;
    }

    return 0;
}
// 0 = identical
// other value, otherwise
int _compare_rt_infos_t(rt_infos_t * rtinfos1,rt_infos_t * rtinfos2)
{
    unsigned int taille1 = ptr_array_length((ptr_array_t *) rtinfos1);
    unsigned int taille2 = ptr_array_length((ptr_array_t *) rtinfos2);
    if(taille1 != taille2)
        return -673;

    unsigned int i = 0;
    for(i=0; i < taille1 ; i++)
    {
        rt_info_t * rtinfo1 =  rtinfos1->data[i];
        rt_info_t * rtinfo2 =  rtinfos2->data[i];

        if(rtinfo1->prefix.mask != rtinfo2->prefix.mask)
            return -674;
        if(rtinfo1->prefix.network != rtinfo2->prefix.network)
            return -674;

        if( rtinfo1->metric != rtinfo2->metric)
            return -675;
        if(rtinfo1->type != rtinfo2->type)
            return -676;
        if ( 0 != _compare_rt_entries(rtinfo1->entries, rtinfo2->entries))
            return -677;
    }

    return 0;
}

// 0 = identical
// other value, otherwise
int _compare_node_rt_t(net_rt_t * rt1, net_rt_t * rt2)
{
    unsigned int num1, num2;
    num1 = trie_num_nodes(rt1,1);
    num2 = trie_num_nodes(rt2,1);
    if(num1 != num2)
        return -671;

    rt_infos_t ** rt_infosss1 = (rt_infos_t **) (_trie_get_array(rt1)->data) ;
    rt_infos_t ** rt_infosss2 = (rt_infos_t **) (_trie_get_array(rt2)->data) ;

    unsigned index;
    for (index= 0; index < num1 ; index++) {
        if( 0 != _compare_rt_infos_t(rt_infosss1[index],rt_infosss2[index]))
          return -672;
    }
    return 0;
}


// 0 = identical
// other value, otherwise
int _routing_info_compare(int nb_nodes, routing_info_t * ri1, routing_info_t * ri2)
{
    // local rib :
    if ( 0 !=  _rib_compare(ri1->bgp_router_loc_rib_t, ri2->bgp_router_loc_rib_t))
        return -600;

    //compare bgp session!

    if( _bgp_sessions_compare(ri1->bgp_sessions_info, ri2->bgp_sessions_info  ) !=0)
        return -650;

    // compare node_rt_t !!!
    
    if ( 0!= (_compare_node_rt_t(ri1->node_rt_t, ri2->node_rt_t )))
    {
        printf("CE FUT UTILE !!!!\n");
        return -670;
    }

    return 0;
}
// 0 = identical
// other value, otherwise
int _routing_states_equivalent(routing_state_t * rs1, routing_state_t * rs2 )
{
  unsigned int i = 0;

  for(i=0 ; i< rs1->state->graph->tracer->nb_nodes ; i++)
  {
     if(_routing_info_compare(rs1->state->graph->tracer->nb_nodes,
             rs1->couples_node_routing_info[i]->routing_info
             ,rs2->couples_node_routing_info[i]->routing_info )
             !=0)
         return -500;
  }
    return 0;
}



#define NETWORK_EDGE_VOID_DOT_STYLE " blablabla "
#define NETWORK_EDGE_FORWARD_DOT_STYLE " blablabla "
#define NETWORK_EDGE_BACKWARD_DOT_STYLE " blablabla "
#define NETWORK_EDGE_BOTH_DOT_STYLE " blablabla "


#define EDGE_TYPE_NONE                  0x0
#define EDGE_TYPE_SIMPLE                0x01
#define EDGE_TYPE_FORWARD               0x02
#define EDGE_TYPE_BACKWARD              0x04
#define EDGE_TYPE_BOTH                  0x06


typedef struct link_type
{
    net_addr_t  from;
    net_addr_t  to;
    uint8_t    type;
}link_type;

    // -----[ net_export_dot ]-------------------------------------------
void routing_state_export_dot(gds_stream_t * stream, routing_state_t * rs)
{
  gds_enum_t * nodes, * subnets;
  net_node_t * node;
  net_subnet_t * subnet;
  net_iface_t * iface;
  unsigned int index;

  //graph_mark_states_for_can_lead_to_final_state(graph);

  //net_export_dot(gdsout, graph->tracer->network);


  // Header
  stream_printf(stream, "/**\n");
  stream_printf(stream, " * State dot file\n");
  stream_printf(stream, " * generated by C-BGP\n");
  stream_printf(stream, " *\n");
  //stream_printf(stream, " * Render with: neato -Tps <file.dot> <file.ps>\n");
  stream_printf(stream, " * Render with: neato <file.dot> -Tpng -o<file.png>\n");
  stream_printf(stream, " */\n");
  stream_printf(stream, "digraph Tracer_state_%u_C_BGP {\n",rs->state->id);

  stream_printf(stream, "  overlap=false\n");
 stream_printf(stream, "  sep=\"+20,20\"\n");

    //size = "100,100";
    //nodesep="0.6";

//#00ABCC   turquoise
//#a80039   bordeau
//#666666   gris
    //stream_printf(stream, "edge [%s]\n", NETWORK_EDGE_VOID_DOT_STYLE);

  // Nodes

  unsigned int i;
  for( i = 0 ; i < rs->state->graph->tracer->nb_nodes ; i++)
  {
    node= rs->couples_node_routing_info[i]->node;
    stream_printf(stream, "  \"");
    node_dump_id(stream, node);
    stream_printf(stream, "\" [");
    //stream_printf(stream, "shape=box");
    stream_printf(stream, "] ;\n");
  }


  // Links

  unsigned int max_m = rs->state->graph->tracer->nb_nodes * (rs->state->graph->tracer->nb_nodes - 1) /2;

  link_type * edges_t[max_m];
  
  unsigned int m ;
  for(m = 0 ; m < max_m ; m++)
  { 
      edges_t[m] = NULL;
  }

  
  for( i = 0 ; i < rs->state->graph->tracer->nb_nodes ; i++)
  {
    couple_node_routinginfo_t * node_rinfo = rs->couples_node_routing_info[i];

    //tous les voisins de ce noeud-ci, si id ">", regarder les info de session bgp
    node = node_rinfo->node;
    routing_info_t * rinfo = node_rinfo->routing_info;
    bgp_sessions_info_t * bgp_sesss_info  = rinfo->bgp_sessions_info;
    unsigned int u;
    for( u = 0 ; u < bgp_sesss_info->nb_bgp_session_info_ ; u++)
    {
        bgp_session_info_t * one_sess_info = bgp_sesss_info->bgp_session_info[u];

        net_addr_t small;
        net_addr_t big;
        unsigned int invert = 0;
        if( node->rid < one_sess_info->neighbor_addr )
        {
            small = node->rid;
            big = one_sess_info->neighbor_addr ;
        }
        else
        {
            small =  one_sess_info->neighbor_addr;
            big = node->rid;
            invert = 1;
        }

        m = 0;
        
        while( m<max_m && edges_t[m] != NULL
                && (edges_t[m]->from!= small || edges_t[m]->to!=big )    )
        {
            m++;
        }
        assert(m<max_m);

        if(edges_t[m] == NULL)
        {
            // l'ajouter en place m !!
            edges_t[m] = (link_type * ) MALLOC (sizeof(link_type));
            edges_t[m]->from = small;
            edges_t[m]->to = big;
            edges_t[m]->type = EDGE_TYPE_SIMPLE;
        }

        // soit on vient de le créer, soit on a trouvé le bon,
        // et il se trouve en place m !

        //1-ajouter flèche simple

        edges_t[m]->type = edges_t[m]->type | EDGE_TYPE_SIMPLE;

        //2-voir si c'est dans ma loc rib, si oui, ajouter une vraie flèche. (attention au sens si invert==1)
        // TODO !!!!

        unsigned int l;
        bgp_route_t * one_route;
        for(l = 0 ; l < rinfo->bgp_router_loc_rib_t->nb_local_rib_elem ; l++)
        {
            one_route = rinfo->bgp_router_loc_rib_t->bgp_route_[l];
            if( one_sess_info->neighbor_addr == one_route->attr->next_hop )
            {
                // ajouter la flèche
                if(invert == 1)
                {
                     edges_t[m]->type = edges_t[m]->type | EDGE_TYPE_BACKWARD;
                }
                else
                {
                    edges_t[m]->type = edges_t[m]->type | EDGE_TYPE_FORWARD;
                }
                break;
            }
        }
    }
  }

  // parcourir le tableau edges, et tracer les liens !

    m = 0;
    while( m<max_m && edges_t[m] != NULL )
    {
       stream_printf(stream, " \"");
       ip_address_dump(stream, edges_t[m]->from);
       stream_printf(stream, "\"");
       stream_printf(stream, "->");
       stream_printf(stream, "\"");
       ip_address_dump(stream, edges_t[m]->to);
       stream_printf(stream, "\"");
       stream_printf(stream, "[ ");
       //stream_printf(stream, "dir=");
       if( edges_t[m]->type &  EDGE_TYPE_FORWARD    &&   edges_t[m]->type & EDGE_TYPE_BACKWARD )
       {
          stream_printf(stream, "dir=both");
       }
       else if( edges_t[m]->type & EDGE_TYPE_FORWARD    )
       {
          stream_printf(stream, "dir=forward");
       }
       else if( edges_t[m]->type & EDGE_TYPE_BACKWARD  )
       {
          stream_printf(stream, "dir=back");
       }
       else
       {
          stream_printf(stream, "dir=none");
       }

       stream_printf(stream, "];\n");

       m++;
    }



/*
  nodes= trie_get_enum(state->graph->tracer->network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
      iface= (net_iface_t *) node->ifaces->data[index];
      switch (iface->type) {
      case NET_IFACE_RTR:
      case NET_IFACE_PTP:

	if (node->rid > iface->dest.iface->owner->rid)
	  continue;
	stream_printf(stream, "  \"");
	node_dump_id(stream, node);
	stream_printf(stream, "\" -- \"");
	node_dump_id(stream, iface->dest.iface->owner);
	stream_printf(stream, "\" [label=\"%u/%u,%u/%u\",dir=both] ;\n",
		      net_iface_get_metric(iface, 0),
		      net_iface_get_metric(iface->dest.iface, 0),
		      net_iface_get_load(iface),
		      net_iface_get_load(iface->dest.iface));
	break;


      default:
	;
      }

    }
  }
  enum_destroy(&nodes);
*/
  stream_printf(stream, "}\n");


}