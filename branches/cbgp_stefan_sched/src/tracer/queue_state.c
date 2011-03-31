

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
#include <tracer/routing_state.h>
#include <tracer/queue_state.h>


#include <sim/tunable_scheduler.h>

#include <libgds/fifo_tunable.h>


#include <libgds/trie.h>
#include <../net/net_types.h>

#include "state.h"
#include "transition.h"



void _queue_state_dump(gds_stream_t * stream, queue_state_t * queue_state)
{
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= queue_state->events->current_depth;
  max_depth= queue_state->events->max_depth;
  start= queue_state->events->start_index;

if(get_state_type_of_dump()!=DUMP_ONLY_CONTENT)
		  stream_printf(stream, "\tQueue State :\n\t\tNumber of events queued: %u (%u)\n",
	     depth, max_depth);

  for (index= 0; index < depth; index++) {
    event= (_event_t *) queue_state->events->items[(start+index) % max_depth];
    if(get_state_type_of_dump()!=DUMP_ONLY_CONTENT)
       stream_printf(stream, "%d\t(%d) ", index, (start+index) % max_depth);
    //stream_printf(stream, "-- Event:%p - ctx:%p - msg:%p - bgpmsg:%p --\n\t\t", event, event->ctx, ((net_send_ctx_t *)event->ctx)->msg,((net_msg_t *)((net_send_ctx_t *)event->ctx)->msg)->payload);
    stream_flush(stream);
    if (event->ops->dump != NULL) {
      event->ops->dump(stream, event->ctx);
    } else {
      stream_printf(stream, "unknown");
    }
    stream_printf(stream, "\n");
  }
}


void _queue_state_flat_simple_HTML_dump(gds_stream_t * stream, queue_state_t * queue_state)
{
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= queue_state->events->current_depth;
  max_depth= queue_state->events->max_depth;
  start= queue_state->events->start_index;

  for (index= 0; index < depth; index++) {
    event= (_event_t *) queue_state->events->items[(start+index) % max_depth];
    //stream_printf(stream, "-- Event:%p - ctx:%p - msg:%p - bgpmsg:%p --\n\t\t", event, event->ctx, ((net_send_ctx_t *)event->ctx)->msg,((net_msg_t *)((net_send_ctx_t *)event->ctx)->msg)->payload);
    stream_flush(stream);
    if (event->ops->dump != NULL ) {
       // event->ops->dump == _network_send_ctx_dump  (from network.c)
      net_send_ctx_t * send_ctx= (net_send_ctx_t *) event->ctx;
         //message_dump(stream, send_ctx->msg);
         const net_protocol_def_t * proto_def;
         ip_address_dump(stream, send_ctx->msg->src_addr);
         stream_printf(stream, " --&gt; ");
         ip_address_dump(stream, send_ctx->msg->dst_addr);
         assert(send_ctx->msg->protocol == NET_PROTOCOL_BGP);
         proto_def= net_protocols_get_def(send_ctx->msg->protocol);
         if (proto_def->ops.dump_msg != NULL){
            //proto_def->ops.dump_msg(stream, msg);
            bgp_msg_flat_simple_HTML_dump(stream,(bgp_msg_t *) send_ctx->msg->payload);
            }
    }
    stream_printf(stream, "<BR/>");
  }
}


net_addr_t get_dst_addr(_event_t * ev)
{
     return ((net_send_ctx_t *) ev->ctx)->msg->dst_addr;
}
net_addr_t get_src_addr(_event_t * ev)
{
     return ((net_send_ctx_t *) ev->ctx)->msg->src_addr;
}


// 0 --> different
// 1 --> same
static int _same_bgp_session_oriented( _event_t * event1, _event_t * event2 )
{
    return (get_dst_addr(event1) == get_dst_addr(event2) &&
            get_src_addr(event1) == get_src_addr(event2));
}


// return the index of next
// or max value if does not exist.
static  int _next_of_bgp_session_oriented(net_addr_t src, net_addr_t dst ,
            gds_fifo_tunable_t * file, unsigned int index, int * visited )
{
    while(index < file->current_depth &&
           (
                visited[index]==1 ||
                get_src_addr((_event_t *) file->items[(file->start_index + index)% file->max_depth]) != src ||
                get_dst_addr((_event_t *) file->items[(file->start_index + index)% file->max_depth]) != dst
           )
         )
        index++;
    return index;
}

unsigned int  _queue_state_calculate_max_nb_of_msg_in_oriented_bgp_session(queue_state_t * qs)
{
    if(qs->max_nb_of_msg_in_one_oriented_session != 0 || qs->events->current_depth == 0)
    {
        return qs->max_nb_of_msg_in_one_oriented_session;
    }


    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    unsigned int nb_msg = 0;
    unsigned int temp = 0;
    int tab1_visited[qs->events->current_depth];

    for(index1=0; index1<qs->events->current_depth ; index1++)
    {
        tab1_visited[index1] = 0;
    }


    while( nb_msg != qs->events->current_depth)
    {
        //trouver le premier message de la session bgp (dirigée) suivante dans file 1:
        index1 = 0;
        while(index1<qs->events->current_depth && tab1_visited[index1]==1)
        {
            index1++;
        }
        assert(index1<qs->events->current_depth);

        // en index1 : premier message d'une session bgp dirigée
        // identifier la session bgp dirigée :
        src = get_src_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        dst = get_dst_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        tab1_visited[index1] = 1;

        nb_msg++;
        temp = 1;

        // la suite de cette session bgp dirigée
        index1 = _next_of_bgp_session_oriented(src,dst,qs->events,index1, tab1_visited);

        while(index1<qs->events->current_depth)
        {
            // dans index1= le suivant de la session bgp dirigée.
            nb_msg++;
            temp++;
            tab1_visited[index1]=1;
            index1 = _next_of_bgp_session_oriented(src,dst,qs->events,index1, tab1_visited);
        }

        assert(index1 == qs->events->current_depth);

        if(temp>qs->max_nb_of_msg_in_one_oriented_session)
            qs->max_nb_of_msg_in_one_oriented_session = temp;
    }
    return qs->max_nb_of_msg_in_one_oriented_session;
}


// ----- state_create ------------------------------------------------
queue_state_t * _queue_state_create(state_t * state, sched_tunable_t * tunable_scheduler)
{
  queue_state_t * queue_state;
  queue_state= (queue_state_t *) MALLOC(sizeof(queue_state_t));

  queue_state->state = state;
  queue_state->events=fifo_tunable_copy( tunable_scheduler->events , fifo_tunable_event_deep_copy   ) ;
  queue_state->max_nb_of_msg_in_one_oriented_session=0;
  _queue_state_calculate_max_nb_of_msg_in_oriented_bgp_session(queue_state);
  queue_state->nb_oriented_bgp_session = 0;
  
  return queue_state;
}

int same_tcp_session(_event_t * event1 , _event_t * event2 )
{
// returns 1 if same tcp session
// 0 otherwise
  net_send_ctx_t * send_ctx1 = (net_send_ctx_t *) event1->ctx;
  net_send_ctx_t * send_ctx2 = (net_send_ctx_t *) event2->ctx;

  net_msg_t * msg1 = send_ctx1->msg;
  net_msg_t * msg2 = send_ctx2->msg;

  if( msg1->src_addr == msg2->src_addr && msg1->dst_addr == msg2->dst_addr)
      return 1;
  else
      return 0;
}



int _queue_state_inject( queue_state_t * queue_state , sched_tunable_t * tunable_scheduler)
{

    // destroy le tunable_scheduler->events,
    // copier l'état actuel dans le scheduler.
  int level=1;     int ilevel; for(ilevel = 0 ; ilevel < level ; ilevel++) if(STATE_DEBBUG==STATE_DEBBUG_YES) printf("  ");
    if(STATE_DEBBUG==STATE_DEBBUG_YES) printf("queue state_inject \n");

     tunable_scheduler->events = fifo_tunable_copy( queue_state->events , fifo_tunable_event_deep_copy  ) ;

     return 1;
}


// 0-->equivalent
// other value otherwize
int _msgs_equivalent(net_msg_t * msg1,net_msg_t * msg2  )
{
    if(msg1->dst_addr != msg2->dst_addr)
        return -1;

    if(msg1->src_addr != msg2->src_addr)
        return -2;

    if( msg1->protocol != msg2->protocol)
        return -3;

    if( msg1->tos != msg2->tos)
        return -4;

    if( msg1->ttl != msg2->ttl)
        return -5;

    const net_protocol_def_t * proto_def;
    proto_def= net_protocols_get_def(msg1->protocol);
    if (proto_def->ops.compare_msg != NULL)
        return proto_def->ops.compare_msg(msg1, msg2);
    else
        return -10;
}

int _events_equivalent( _event_t * event1, _event_t * event2   )
{
    net_send_ctx_t * send_ctx1= (net_send_ctx_t *) event1->ctx;
    net_send_ctx_t * send_ctx2= (net_send_ctx_t *) event2->ctx;

    if( send_ctx1->dst_iface->addr != send_ctx2->dst_iface->addr)
        return -1;

    return _msgs_equivalent(send_ctx1->msg, send_ctx2->msg);
}


// 0 = identical
// other value, otherwise
int _queue_states_equivalent(queue_state_t * qs1, queue_state_t  * qs2 )
{
    if(qs1->events->current_depth != qs2->events->current_depth )
    {
        return -1;
    }

    unsigned int i;

    for(i = 0 ; i < qs1->events->current_depth ; i++)
    {
        if( 0 != _events_equivalent(
                (_event_t *) qs1->events->items[(qs1->events->start_index + i)% qs1->events->max_depth],
                (_event_t *) qs2->events->items[(qs2->events->start_index + i)% qs2->events->max_depth]
                ))
            return -1;
    }
    return 0;
}

// 0 = identical
// other value, otherwise
int _queue_states_equivalent_v2(queue_state_t * qs1, queue_state_t  * qs2 )
{
    if(qs1->events->current_depth != qs2->events->current_depth )
    {
        return -1;
    }
    if(qs1->max_nb_of_msg_in_one_oriented_session != qs2->max_nb_of_msg_in_one_oriented_session)
    {
        return -2;
    }

    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    unsigned int index2;
    unsigned int nb_msg = 0;
    //int * tab1_visited = (int *) MALLOC ( qs1->events->current_depth * sizeof(int));
    //int * tab2_visited = (int *) MALLOC ( qs1->events->current_depth * sizeof(int));
    int tab1_visited[qs1->events->current_depth];
    int tab2_visited[qs1->events->current_depth];

    for(index1=0; index1<qs1->events->current_depth ; index1++)
    {
        tab1_visited[index1] = 0;
        tab2_visited[index1] = 0;
    }


    while( nb_msg != qs1->events->current_depth)
    {
        //trouver le premier message de la session bgp (dirigée) suivante dans file 1:
        index1 = 0;
        while(index1<qs1->events->current_depth && tab1_visited[index1]==1)
        {
            index1++;
        }
        assert(index1<qs1->events->current_depth);

        // en index1 : premier message d'une session bgp dirigée
        // identifier la session bgp dirigée :
        src = get_src_addr((_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth]);
        dst = get_dst_addr((_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth]);
        tab1_visited[index1] = 1;
        // trouver le premier message de file2 qui est de cette session bgp.
        index2 = 0;
        while(index2<qs1->events->current_depth &&
                (tab2_visited[index2]==1 || !_same_bgp_session_oriented(
                     (_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth],
                     (_event_t *) qs2->events->items[(qs2->events->start_index + index2)% qs2->events->max_depth]))
             )
        {
           index2++;
        }
        // we go out of the while wheter index2 too high, or we found the msg !
        if(index2==qs1->events->current_depth)
        {
            return -800;
        }

        if(0 != _events_equivalent( (_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth],
                                    (_event_t *) qs2->events->items[(qs2->events->start_index + index2)% qs2->events->max_depth]))
        {
            return -810;
        }

        nb_msg++;
        tab2_visited[index2]=1;

        // la suite de cette session bgp dirigée
        index1 = _next_of_bgp_session_oriented(src,dst,qs1->events,index1, tab1_visited);
        index2 = _next_of_bgp_session_oriented(src,dst,qs2->events,index2, tab2_visited);

        while(index1<qs1->events->current_depth && index2<qs2->events->current_depth)
        {
            // dans index1= le suivant de la session bgp dirigée.
            // dans index2= le suivant de la session bgp dirigée.
            if(0 != _events_equivalent( (_event_t *) qs1->events->items[(qs1->events->start_index + index1)% qs1->events->max_depth],
                                    (_event_t *) qs2->events->items[(qs2->events->start_index + index2)% qs2->events->max_depth]))
            {
                return -820;
            }
            nb_msg++;
            tab1_visited[index1]=1;
            tab2_visited[index2]=1;
            index1 = _next_of_bgp_session_oriented(src,dst,qs1->events,index1, tab1_visited);
            index2 = _next_of_bgp_session_oriented(src,dst,qs2->events,index2, tab2_visited);
        }

        // soit il n'y en a plus dans les deux files, et c'est ok
        if( index1 != qs1->events->current_depth || index2 != qs2->events->current_depth)
            return -830;

    }
    return 0;
}

unsigned int count_nb_oriented_sessions(queue_state_t * qs)
{

    if(qs->nb_oriented_bgp_session != 0 ||  qs->events->current_depth == 0)
    {
        return qs->nb_oriented_bgp_session;
    }

    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    unsigned int nb_msg = 0;
    qs->nb_oriented_bgp_session = 0;
    int tab1_visited[qs->events->current_depth];

    for(index1=0; index1<qs->events->current_depth ; index1++)
    {
        tab1_visited[index1] = 0;
    }


    while( nb_msg != qs->events->current_depth)
    {
        //trouver le premier message de la session bgp (dirigée) suivante dans file 1:
        index1 = 0;
        while(index1<qs->events->current_depth && tab1_visited[index1]==1)
        {
            index1++;
        }
        assert(index1<qs->events->current_depth);

        // en index1 : premier message d'une session bgp dirigée
        // identifier la session bgp dirigée :
        src = get_src_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        dst = get_dst_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        tab1_visited[index1] = 1;

        nb_msg++;
        (qs->nb_oriented_bgp_session)++;

        // la suite de cette session bgp dirigée
        index1 = _next_of_bgp_session_oriented(src,dst,qs->events,index1, tab1_visited);

        while(index1<qs->events->current_depth)
        {
            // dans index1= le suivant de la session bgp dirigée.
            nb_msg++;
            tab1_visited[index1]=1;
            index1 = _next_of_bgp_session_oriented(src,dst,qs->events,index1, tab1_visited);
        }

        assert(index1 == qs->events->current_depth);
    }

    // nb de sessions : nb_session_oriented
    return qs->nb_oriented_bgp_session;
}

session_waiting_time_t ** create_session_waiting_time_container(queue_state_t * qs)
{

   
   // 1er parcours : calculer
    count_nb_oriented_sessions(qs);
     // 2eme parcours = remplir

    // technique à revoir pour améliorer la complexité.!!!

    net_addr_t src;
    net_addr_t dst;

    unsigned int index1;
    session_waiting_time_t **  swt  = (session_waiting_time_t **) MALLOC ( qs->nb_oriented_bgp_session * sizeof(session_waiting_time_t *));
    unsigned int current_nb_of_sessions = 0;
    unsigned int i = 0;

    for(index1 = 0 ; index1 <  qs->events->current_depth ; index1++)
    {
        src = get_src_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);
        dst = get_dst_addr((_event_t *) qs->events->items[(qs->events->start_index + index1)% qs->events->max_depth]);

        // chercher parmis ceux déjà présents pour savoir si on a déjà ajouté cette session
        for(i = 0 ; i < current_nb_of_sessions && (swt[i]->from != src || swt[i]->to != dst ); i++)
        {
        }
        // non présent
        if(i == current_nb_of_sessions )
        {
            swt[current_nb_of_sessions] = (session_waiting_time_t *) MALLOC ( sizeof(session_waiting_time_t ));
            swt[current_nb_of_sessions]->from = src;
            swt[current_nb_of_sessions]->to = dst;
            swt[current_nb_of_sessions]->max_waiting_time=-1;
            swt[current_nb_of_sessions]->min_waiting_time=-1;
            current_nb_of_sessions++;
        }
    }
    return swt;
}