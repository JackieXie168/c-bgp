
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>

#include <libgds/memory.h>
#include <libgds/stack.h>

#include <time.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#include <tracer/tracer.h>
#include <tracer/graph.h>
#include <tracer/state.h>

#include "graph.h"
#include "state.h"
#include "transition.h"
#include "tracer.h"
#include "queue_state.h"



// ----- state_create ------------------------------------------------
graph_t * graph_create(tracer_t * tracer)
{
  graph_t * graph;
  graph=(graph_t *) MALLOC(sizeof(graph_t));

  graph->tracer=tracer;
  graph->state_root= NULL;
  //graph->FOR_TESTING_PURPOSE_current_state = NULL;
  graph->nb_states = 0;
  graph->nb_final_states = 0;

  graph->list_of_states = (state_t **) MALLOC( MAX_STATE * sizeof(state_t *) );
  unsigned int i;
  for (i=0;i<MAX_STATE;i++)
      graph->list_of_states[i]=NULL;
  graph->list_of_final_states = (state_t **) MALLOC( MAX_FINAL_STATES * sizeof(state_t *) );
  for (i=0;i<MAX_FINAL_STATES;i++)
      graph->list_of_final_states[i]=NULL;

  graph->marking_sequence_number = 1;

  return graph;
}

int graph_add_state(graph_t * the_graph, struct state_t * the_state, unsigned int index)
{
    assert (index == the_graph->nb_states);
    if(index<MAX_STATE)
    {
        the_graph->list_of_states[index]=the_state;
        (the_graph->nb_states)++;
        return 0;
    }
    else
    {   printf("MAX number states reached !\n");
        return -1;
    }
}


int graph_add_final_state(graph_t * the_graph, struct state_t * the_state)
{
    //assert (index == the_graph->nb_states);
    if( the_graph->nb_final_states < MAX_FINAL_STATES )
    {
        the_graph->list_of_final_states[the_graph->nb_final_states]=the_state;
        (the_graph->nb_final_states)++;
        return 0;
    }
    else
    {   printf("MAX number of final states reached !\n");
        return -1;
    }
}

// ----- state_create ------------------------------------------------
int graph_init(graph_t * graph)
{
    // nettoyer le reste si déjà été utilisé ... (pas pour l'instant)
  assert(graph->state_root == NULL && graph->nb_states == 0);
  graph->state_root= state_create_isolated(graph->tracer);
  state_attach_to_graph(graph->state_root, NULL);

  //graph->FOR_TESTING_PURPOSE_current_state = graph->state_root;
  //graph->nb_states = 1;
  graph->state_root->type = graph->state_root->type | STATE_ROOT;
  //struct state_t        **    list_of_states;
  return 0;
}

int graph_root_dump(gds_stream_t * stream, graph_t * graph)
{
    return state_dump( stream, graph->state_root);
}


int FOR_TESTING_PURPOSE_graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state)
{
    stream_printf(stream,"Imprimer l'état %d\n",num_state);
    state_t * current_state = graph->state_root;
    unsigned int i = 0 ;
    
    while( i < num_state)
    {   
        current_state = current_state->output_transitions[0]->to;
        i++;
    }
    
    return state_dump( stream, current_state);
}

/*int FOR_TESTING_PURPOSE_graph_current_state_dump(gds_stream_t * stream, graph_t * graph)
{
    return state_dump( stream, graph->FOR_TESTING_PURPOSE_current_state);
}*/

int graph_mark_states_for_can_lead_to_final_state(graph_t * graph)
{
    // prendre chaque état final, et leur dire de marquer leur input_transition comme can lead to final state.
    graph->marking_sequence_number++;
    unsigned int i;

    for(i = 0 ; i < graph->nb_final_states ; i++)
    {
        state_mark_for_can_lead_to_final_state(graph->list_of_final_states[i],graph->marking_sequence_number );
    }
    return graph->marking_sequence_number;
}

int graph_allstates_dump(gds_stream_t * stream, graph_t * graph)
{
    stream_printf(stream,"***************\n\nImprimer tous les états \n");
    
    unsigned int i = 0 ;

    while(i <  MAX_STATE && graph->list_of_states[i] != NULL)
    {
        //stream_printf(stream,"\nEtat %d :\n ",i);
        state_dump( stream, graph->list_of_states[i]);

        i++;
        

        /*
        if(current_state->output_transitions != NULL && current_state->output_transitions[0] != NULL)
            current_state = current_state->output_transitions[0]->to;
        else
            current_state=NULL;
        */
        stream_printf(stream,"\n***************\n\n");

    }

    return 1;
}

/*
    int FOR_TESTING_PURPOSE_graph_inject_state(graph_t * graph , unsigned int num_state)
    {
        
    state_t * current_state = graph->state_root;
    unsigned int i = 0 ;
// ici c'est une chaine (pas encore un arbre)
    while( i < num_state)
    {
        current_state = current_state->output_transitions[0]->to;
        i++;
    }

    //current_state est l'état à injecter.

     return state_inject(current_state);

    }
*/

    unsigned int graph_max_num_state()
    {
        return MAX_STATE;
    }

    int graph_inject_state(graph_t * graph , unsigned int num_state)
    {
        state_t * state;
         if(num_state >= graph_max_num_state())
         {
            printf("ATTENTION ! nombre max d'état = %d\n",graph_max_num_state());
            return -1;
        }
        assert(graph->list_of_states != NULL );

        state = graph->list_of_states[num_state];

        if(state == NULL)
        {
            printf("Unexisting state !\n");
            return -2;
        }

        return state_inject(state);
    }
    int graph_state_dump(gds_stream_t * stream, graph_t * graph, unsigned int num_state)
    {
        state_t * state;
         if(num_state >= graph_max_num_state())
         {
            printf("ATTENTION ! nombre max d'état = %d\n",graph_max_num_state());
            return -1;
        }
        assert(graph->list_of_states != NULL );

        state = graph->list_of_states[num_state];

        if(state == NULL)
        {
            printf("Unexisting state !\n");
            return -2;
        }

        return state_dump(stream,state);
    }

    state_t * graph_search_identical_state(graph_t * graph, struct state_t * state)
    {
        unsigned int i = 0 ;

        while(i <  MAX_STATE && graph->list_of_states[i] != NULL)
        {
            //stream_printf(stream,"\nEtat %d :\n ",i);
           if( state_identical(state , graph->list_of_states[i]) == 0 )
               return graph->list_of_states[i];
            i++;
        }
        return NULL;
    }


static void transition_short_dump(gds_stream_t * stream, transition_t * trans)
{
    _event_t * ev =  (_event_t *)trans->event;
    net_send_ctx_t * sendctx = ev->ctx;
    net_msg_t * msg = sendctx->msg;

    stream_printf(stream, "");
    ip_address_dump(stream,msg->src_addr);
    stream_printf(stream, "\\n->");
    ip_address_dump(stream,msg->dst_addr);
    stream_printf(stream, "");


}

// -----[ net_export_dot ]-------------------------------------------
void graph_export_dot(gds_stream_t * stream, graph_t * graph)
{
  gds_enum_t * nodes, * subnets;
  net_node_t * node;
  net_subnet_t * subnet;
  net_iface_t * iface;
  unsigned int index;
    
  graph_mark_states_for_can_lead_to_final_state(graph);

  //net_export_dot(gdsout, graph->tracer->network);

  // Header
  stream_printf(stream, "/**\n");
  stream_printf(stream, " * StateGraph  dot file\n");
  stream_printf(stream, " * generated by C-BGP\n");
  stream_printf(stream, " *\n");
  //stream_printf(stream, " * Render with: neato -Tps <file.dot> <file.ps>\n");
  stream_printf(stream, " * Render with: dot <file.dot> -Tpng -o<file.png>\n");
  stream_printf(stream, " */\n");
  stream_printf(stream, "digraph Tracer_C_BGP {\n");
  stream_printf(stream, "rankdir=LR;\n\n");


  //stream_printf(stream, "  overlap=scale\n");

    //size = "100,100";
    //nodesep="0.6";

//#00ABCC   turquoise
//#a80039   bordeau
//#666666   gris
    stream_printf(stream, "edge [%s]\n",GRAPH_EDGE_DOT_STYLE);


    //each state : its id
    unsigned int i;
    for(i=0; i< graph->nb_states; i++)
    {
      stream_printf(stream, "%u ",graph->list_of_states[i]->id );

      stream_printf(stream, "[");
      if( graph->list_of_states[i]->type & STATE_ROOT  )
        stream_printf(stream, "%s,", STATE_ROOT_DOT_STYLE);
      
      if( graph->list_of_states[i]->type & STATE_FINAL  )
                stream_printf(stream, "%s,", STATE_FINAL_DOT_STYLE);
//             stream_printf(stream, "shape=%s, style=filled,color=\"%s\",", STATE_FINAL_SHAPE, STATE_FINAL_COLOR);
      else if( graph->list_of_states[i]->type & STATE_CAN_LEAD_TO_A_FINAL_STATE  )
        stream_printf(stream, "%s,",  STATE_CAN_LEAD_TO_A_FINAL_STATE_DOT_STYLE);

      if(graph->list_of_states[i]->nb_allowed_output_transitions != graph->list_of_states[i]->nb_output)
      {
        stream_printf(stream, "%s,",  STATE_NOT_COMPLETELY_TREATED_DOT_STYLE);
        //printf("node %d :  allowed : %d, nboutput ;%d\n",graph->list_of_states[i]->id,graph->list_of_states[i]->allowed_output_transitions,graph->list_of_states[i]->nb_output);
      }

      /*stream_printf(stream, "label=\"%u\\n%u msg\",",graph->list_of_states[i]->id,graph->list_of_states[i]->queue_state->events->current_depth);
      stream_printf(stream, " image=\"%s_state_%u.dot.ps\", ",graph->tracer->base_output_file_name,graph->list_of_states[i]->id);
      stream_printf(stream, " labelloc=b ");
*/
      //stream_printf(stream, "label=<<TABLE><TR><TD><IMG SRC=\"%s_state_%u.dot.png\"/></TD></TR><TR><TD><b>%u</b></TD><TD>%u msg</TD></TR></TABLE>>",graph->tracer->base_output_file_name,graph->list_of_states[i]->id,graph->list_of_states[i]->id,graph->list_of_states[i]->queue_state->events->current_depth );
      //stream_printf(stream, "label=<<IMG SRC=\"%s_state_%u.dot.png\"/><BR/><B>%u</B><BR/>%u msg : <BR/>",graph->tracer->base_output_file_name,graph->list_of_states[i]->id,graph->list_of_states[i]->id,graph->list_of_states[i]->queue_state->events->current_depth );


    // si l'image existe
      int retour;
      char commande[1024];
      sprintf(commande,"if [ -f \"%s%s_state_%u.dot.%s\" ];then exit 0; else exit 1; fi;",graph->tracer->base_output_directory,graph->tracer->base_output_file_name,graph->list_of_states[i]->id, graph->tracer->IMAGE_FORMAT);
      retour = system(commande);
      unsigned int affichage = 1;

      stream_printf(stream, "label=<<TABLE BORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\">");
      stream_printf(stream, "<TR><TD><B>STATE %u</B><BR/>Profondeur : %u</TD></TR>",graph->list_of_states[i]->id ,graph->list_of_states[i]->depth );

      if(retour == 0  && affichage !=0 )
      {
          //printf("image OK!! \n");
          stream_printf(stream, "<TR><TD><IMG SRC=\"%s%s_state_%u.dot.%s\"/></TD></TR>",graph->tracer->base_output_directory,graph->tracer->base_output_file_name,graph->list_of_states[i]->id, graph->tracer->IMAGE_FORMAT);
      }
      stream_printf(stream, "<TR><TD>%u msg</TD></TR>",graph->list_of_states[i]->queue_state->events->current_depth );
      stream_printf(stream, "<TR><TD>");
      _queue_state_flat_simple_HTML_dump(stream,graph->list_of_states[i]->queue_state);
      stream_printf(stream, "</TD></TR>");
      stream_printf(stream, "<TR><TD>");
      state_tag_waiting_time_HTML_dump(stream, graph->list_of_states[i]);
      stream_printf(stream, "</TD></TR>");


      stream_printf(stream, "</TABLE>>,");

      if(retour == 0 )
          stream_printf(stream, " URL=\"%s_state_%u.dot.%s\" ",graph->tracer->base_output_file_name,graph->list_of_states[i]->id,graph->tracer->IMAGE_FORMAT);

      stream_printf(stream, "]\n");
    }

    // each state : its transition (with a state at the end of the transition ...

    for(i=0; i< graph->nb_states; i++)
    {
      //
       state_t * state = graph->list_of_states[i];
       unsigned int tra;
       for (tra = 0 ; tra < state->nb_output ; tra++)
       {
        if(state->output_transitions[tra]->to != NULL)
        {
            stream_printf(stream, "%u -> %u [",state->id, state->output_transitions[tra]->to->id);
        
            if(TRANSITION_DOT_DUMP_VERSION == TRANSITION_DOT_DUMP_VERSION_ID)
            {
                stream_printf(stream, " taillabel=\"%u\" ", state->output_transitions[tra]->num_trans);
            }
            else if(TRANSITION_DOT_DUMP_VERSION == TRANSITION_DOT_DUMP_VERSION_SHORT_DUMP)
            {
               stream_printf(stream, " taillabel=\"");
                transition_short_dump(stream,state->output_transitions[tra]);
                stream_printf(stream, "\"");
            }
            stream_printf(stream, "];\n");
        }
       }
    }

  stream_printf(stream, "}\n");

  /*
  

  // Nodes
  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    stream_printf(stream, "  \"");
    node_dump_id(stream, node);
    stream_printf(stream, "\" [");
    stream_printf(stream, "shape=box");
    stream_printf(stream, "] ;\n");
  }
  enum_destroy(&nodes);

  // Subnets
  subnets= _array_get_enum((array_t *) network->subnets);
  while (enum_has_next(subnets)) {
    subnet= *((net_subnet_t **) enum_get_next(subnets));
    stream_printf(stream, "  \"");
    subnet_dump_id(stream, subnet);
    stream_printf(stream, "\" [");
    stream_printf(stream, "shape=ellipse");
    stream_printf(stream, "] ;\n");
  }
  enum_destroy(&subnets);

  // Links
  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
      iface= (net_iface_t *) node->ifaces->data[index];
      switch (iface->type) {
      case NET_IFACE_RTR:
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
      case NET_IFACE_PTMP:
	stream_printf(stream, "  \"");
	node_dump_id(stream, node);
	stream_printf(stream, "\" -- \"");
	subnet_dump_id(stream, iface->dest.subnet);
	stream_printf(stream, "\" [label=\"%u,%u\",dir=forward] ;\n",
		      net_iface_get_metric(iface, 0),
		      net_iface_get_load(iface));
	break;
      default:
	;
      }

    }
  }
  enum_destroy(&nodes);

  stream_printf(stream, "}\n");
   * */
  
}


// -----[ net_export_dot ]-------------------------------------------
void graph_export_dot_SIMPLE(gds_stream_t * stream, graph_t * graph)
{
  gds_enum_t * nodes, * subnets;
  net_node_t * node;
  net_subnet_t * subnet;
  net_iface_t * iface;
  unsigned int index;

  graph_mark_states_for_can_lead_to_final_state(graph);

  //net_export_dot(gdsout, graph->tracer->network);

  // Header
  stream_printf(stream, "/**\n");
  stream_printf(stream, " * StateGraph  dot file\n");
  stream_printf(stream, " * generated by C-BGP\n");
  stream_printf(stream, " *\n");
  //stream_printf(stream, " * Render with: neato -Tps <file.dot> <file.ps>\n");
  stream_printf(stream, " * Render with: dot <file.dot> -Tpng -o<file.png>\n");
  stream_printf(stream, " */\n");
  stream_printf(stream, "digraph Tracer_C_BGP {\n");

  //stream_printf(stream, "  overlap=scale\n");

    //size = "100,100";
    //nodesep="0.6";

//#00ABCC   turquoise
//#a80039   bordeau
//#666666   gris
    stream_printf(stream, "edge [%s]\n",GRAPH_EDGE_DOT_STYLE);


    //each state : its id
    unsigned int i;
    for(i=0; i< graph->nb_states; i++)
    {
      stream_printf(stream, "%u ",graph->list_of_states[i]->id );

      stream_printf(stream, "[");
      if( graph->list_of_states[i]->type & STATE_ROOT  )
        stream_printf(stream, "%s,", STATE_ROOT_DOT_STYLE);

      if( graph->list_of_states[i]->type & STATE_FINAL  )
                stream_printf(stream, "%s,", STATE_FINAL_DOT_STYLE);
//             stream_printf(stream, "shape=%s, style=filled,color=\"%s\",", STATE_FINAL_SHAPE, STATE_FINAL_COLOR);
      else if( graph->list_of_states[i]->type & STATE_CAN_LEAD_TO_A_FINAL_STATE  )
        stream_printf(stream, "%s,",  STATE_CAN_LEAD_TO_A_FINAL_STATE_DOT_STYLE);

      if(graph->list_of_states[i]->nb_allowed_output_transitions != graph->list_of_states[i]->nb_output)
      {
        stream_printf(stream, "%s,",  STATE_NOT_COMPLETELY_TREATED_DOT_STYLE);
        //printf("node %d :  allowed : %d, nboutput ;%d\n",graph->list_of_states[i]->id,graph->list_of_states[i]->allowed_output_transitions,graph->list_of_states[i]->nb_output);
      }

      /*stream_printf(stream, "label=\"%u\\n%u msg\",",graph->list_of_states[i]->id,graph->list_of_states[i]->queue_state->events->current_depth);
      stream_printf(stream, " image=\"%s_state_%u.dot.ps\", ",graph->tracer->base_output_file_name,graph->list_of_states[i]->id);
      stream_printf(stream, " labelloc=b ");
*/
      //stream_printf(stream, "label=<<TABLE><TR><TD><IMG SRC=\"%s_state_%u.dot.png\"/></TD></TR><TR><TD><b>%u</b></TD><TD>%u msg</TD></TR></TABLE>>",graph->tracer->base_output_file_name,graph->list_of_states[i]->id,graph->list_of_states[i]->id,graph->list_of_states[i]->queue_state->events->current_depth );
      //stream_printf(stream, "label=<<IMG SRC=\"%s_state_%u.dot.png\"/><BR/><B>%u</B><BR/>%u msg : <BR/>",graph->tracer->base_output_file_name,graph->list_of_states[i]->id,graph->list_of_states[i]->id,graph->list_of_states[i]->queue_state->events->current_depth );


    // si l'image existe
      int retour;
      char commande[1024];
      sprintf(commande,"if [ -f \"%s%s_state_%u.dot.%s\" ];then exit 0; else exit 1; fi;",graph->tracer->base_output_directory,graph->tracer->base_output_file_name,graph->list_of_states[i]->id, graph->tracer->IMAGE_FORMAT);
      retour = system(commande);
      unsigned int affichage = 1;

      /*
      stream_printf(stream, "label=<<TABLE BORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\">");
      stream_printf(stream, "<TR><TD><B>STATE %u</B><BR/>Profondeur : %u</TD></TR>",graph->list_of_states[i]->id ,graph->list_of_states[i]->depth );

      if(retour == 0  && affichage !=0 )
      {
          //printf("image OK!! \n");
          stream_printf(stream, "<TR><TD><IMG SRC=\"%s%s_state_%u.dot.%s\"/></TD></TR>",graph->tracer->base_output_directory,graph->tracer->base_output_file_name,graph->list_of_states[i]->id, graph->tracer->IMAGE_FORMAT);
      }
      stream_printf(stream, "<TR><TD>%u msg</TD></TR>",graph->list_of_states[i]->queue_state->events->current_depth );
      stream_printf(stream, "<TR><TD>");
      _queue_state_flat_simple_HTML_dump(stream,graph->list_of_states[i]->queue_state);
      stream_printf(stream, "</TD></TR>");
      stream_printf(stream, "<TR><TD>");
      state_tag_waiting_time_HTML_dump(stream, graph->list_of_states[i]);
      stream_printf(stream, "</TD></TR>");


      stream_printf(stream, "</TABLE>>,");
      */

      if(retour == 0 )
          stream_printf(stream, " URL=\"%s_state_%u.dot.%s\" ",graph->tracer->base_output_file_name,graph->list_of_states[i]->id,graph->tracer->IMAGE_FORMAT);

      stream_printf(stream, "]\n");
    }

    // each state : its transition (with a state at the end of the transition ...

    for(i=0; i< graph->nb_states; i++)
    {
      //
       state_t * state = graph->list_of_states[i];
       unsigned int tra;
       for (tra = 0 ; tra < state->nb_output ; tra++)
       {
        if(state->output_transitions[tra]->to != NULL)
        {
            stream_printf(stream, "%u -> %u [",state->id, state->output_transitions[tra]->to->id);

            if(TRANSITION_DOT_DUMP_VERSION == TRANSITION_DOT_DUMP_VERSION_ID)
            {
                stream_printf(stream, " taillabel=\"%u\" ", state->output_transitions[tra]->num_trans);
            }
            else if(TRANSITION_DOT_DUMP_VERSION == TRANSITION_DOT_DUMP_VERSION_SHORT_DUMP)
            {
               stream_printf(stream, " taillabel=\"");
                transition_short_dump(stream,state->output_transitions[tra]);
                stream_printf(stream, "\"");
            }
            stream_printf(stream, "];\n");
        }
       }
    }

  stream_printf(stream, "}\n");

  /*


  // Nodes
  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    stream_printf(stream, "  \"");
    node_dump_id(stream, node);
    stream_printf(stream, "\" [");
    stream_printf(stream, "shape=box");
    stream_printf(stream, "] ;\n");
  }
  enum_destroy(&nodes);

  // Subnets
  subnets= _array_get_enum((array_t *) network->subnets);
  while (enum_has_next(subnets)) {
    subnet= *((net_subnet_t **) enum_get_next(subnets));
    stream_printf(stream, "  \"");
    subnet_dump_id(stream, subnet);
    stream_printf(stream, "\" [");
    stream_printf(stream, "shape=ellipse");
    stream_printf(stream, "] ;\n");
  }
  enum_destroy(&subnets);

  // Links
  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
      iface= (net_iface_t *) node->ifaces->data[index];
      switch (iface->type) {
      case NET_IFACE_RTR:
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
      case NET_IFACE_PTMP:
	stream_printf(stream, "  \"");
	node_dump_id(stream, node);
	stream_printf(stream, "\" -- \"");
	subnet_dump_id(stream, iface->dest.subnet);
	stream_printf(stream, "\" [label=\"%u,%u\",dir=forward] ;\n",
		      net_iface_get_metric(iface, 0),
		      net_iface_get_load(iface));
	break;
      default:
	;
      }

    }
  }
  enum_destroy(&nodes);

  stream_printf(stream, "}\n");
   * */

}



    void graph_export_allStates_to_file(graph_t * graph)
    {
        unsigned int i;
        for(i=0; i< graph->nb_states; i++)
        {
           state_export_to_file(graph->list_of_states[i]);
        }

    }

   void graph_export_allStates_dot_to_file(graph_t * graph)
    {
        unsigned int i;
        for(i=0; i< graph->nb_states; i++)
        {
           state_export_dot_to_file(graph->list_of_states[i]);
        }

    }



int graph_export_dot_to_file(graph_t * graph)
{
    char file_name[256];
    sprintf(file_name,"%s%s_graph_SIMPLE.dot",graph->tracer->base_output_directory,graph->tracer->base_output_file_name);
    
    gds_stream_t * stream = stream_create_file(file_name);

    graph_export_dot_SIMPLE(stream,graph);

    stream_destroy(&stream);
    char commande[1024];
    sprintf(commande,"dot -T%s %s -o%s.%s",graph->tracer->IMAGE_FORMAT,file_name, file_name,graph->tracer->IMAGE_FORMAT);
    //sprintf(commande,"dot -Tps %s -o%s.ps",file_name, file_name);
    system(commande);

    sprintf(file_name,"%s%s_graph.dot",graph->tracer->base_output_directory,graph->tracer->base_output_file_name);
    stream = stream_create_file(file_name);
    graph_export_dot(stream,graph);
    stream_destroy(&stream);
    sprintf(commande,"dot -T%s %s -o%s.%s",graph->tracer->IMAGE_FORMAT,file_name, file_name,graph->tracer->IMAGE_FORMAT);
    //sprintf(commande,"dot -Tps %s -o%s.ps",file_name, file_name);
    system(commande);
    

    return 0;
}


state_t * get_state_with_mininum_bigger_number_of_msg_in_session(graph_t * graph)
{
    //se balader parmi tous les états 'actifs' cad non complètement traités, cad nballowed > nboutput
   unsigned int i;
   assert(graph->nb_states > 0);
   state_t * min_state = NULL;

   for(i = 0 ; i < graph->nb_states ; i++)
   {
       if(state_calculate_allowed_output_transitions(graph->list_of_states[i]) > graph->list_of_states[i]->nb_output
               && !(graph->list_of_states[i]->blocked & STATE_DEFINITELY_BLOCKED) )
       {
           if(min_state == NULL || 
                   graph->list_of_states[i]->queue_state->max_nb_of_msg_in_one_oriented_session
                   < min_state->queue_state->max_nb_of_msg_in_one_oriented_session)
               min_state = graph->list_of_states[i];
       }
   }
   return min_state;
}
