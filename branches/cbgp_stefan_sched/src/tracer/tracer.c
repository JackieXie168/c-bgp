

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <time.h>


#include <assert.h>
#include <libgds/str_util.h>
#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/fifo.h>
#include <libgds/lifo.h>


#include <libgds/memory.h>
#include <libgds/trie.h>


#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#include <tracer/transition.h>
#include <tracer/tracer.h>
#include <tracer/graph.h>
#include <tracer/state.h>
#include <tracer/transition.h>

#include "tracer.h"
#include "state.h"
#include "transition.h"
#include "graph.h"


#define IMAGE_GRAPHVIZ_FORMAT "eps"

#define VERSION_1 1
#define VERSION_2 2
#define STATE_STORAGE_METHOD VERSION_2


static tracer_t  * _default_tracer= NULL;

/*
int FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_t * self )
{
    if(self->started == 0)
        return -1;

    //actuellement graphe est une simple chaine
    // injecter l'état actuel dans le système (scheduler ou simulator)
    // faire simuler un pas (le premier message sera celui traité)
    // capturer l'état actuel, le mettre dans un nouvel état et le lier au précédent.
    // noter le nouvel état comme l'état actuel.

    state_t * current_state = self->graph->FOR_TESTING_PURPOSE_current_state;
    // injecter l'état
    // ici ne rien faire car c'est juste une liste, la file n'a pas bougé
    //construire la transition avec le prochain event qui sera simulé
    transition_t * transition = transition_create_from(get_event_at((struct sched_t *)tracer_get_tunable_scheduler(self),0),current_state, 0);
    state_add_output_transition(current_state,transition);
    //simuler le prochain événement.
    sim_step(tracer_get_simulator(self), 1);
    //construire l'état résultant.
    state_t * new_state = state_create(self,transition);

    self->graph->FOR_TESTING_PURPOSE_current_state = new_state;    

    return 1;
}
 * */


typedef struct state_trans_t {
  unsigned int state;
  unsigned int trans;
} state_trans_t;

void destroy_struct_state_trans(void ** structstatetrans)
{
    //FREE(structstatetrans);
}



/**
 * 1 : FILTER_OK
 * 0 : FILTER_NOK
 *
 * @param state
 * @param depth
 * @return
 */
int filter_limit_on_depth(state_t * state, unsigned int depth)
{
    if(state->depth<depth)
        return FILTER_OK;
    else
        return FILTER_NOK;
}

/*
int filter_limit_on_diff_depth(tracer_t * tracer, state_t * state)
{
    if(state->depth <= tracer->graph->list_of_states[tracer->initial_state_for_tracing]->depth + tracer->filter_diff_depth)
        return FILTER_OK;
    else
        return FILTER_NOK;
}
*/

int filter_limit_on_max_nb_msg_in_oriented_bgp_session(state_t * state, unsigned int nbmsgmax)
{
    if( _queue_state_calculate_max_nb_of_msg_in_oriented_bgp_session(state->queue_state) < nbmsgmax)
        return FILTER_OK;
    else
        return FILTER_NOK;
}


void filter_set_limit_on_depth(tracer_t * self, unsigned int depth)
{
    self->filter->depth = depth;
}

void filter_unset_limit_on_depth(tracer_t * self)
{
    self->filter->depth = 0;
}

void filter_set_limit_on_diff_depth(tracer_t * self, unsigned int depth)
{
    self->filter->diff_depth = depth;
}

void filter_unset_limit_on_diff_depth(tracer_t * self)
{
    self->filter->depth = 0;
}

void filter_set_limit_on_max_nb_in_oriented_session(tracer_t * self, unsigned int nb_msg)
{
    self->filter->nb_max_msg = nb_msg;
}

void filter_unset_limit_on_max_nb_in_oriented_session(tracer_t * self)
{
    self->filter->nb_max_msg = 0;
}


void tracer_set_initial_state_for_tracing(tracer_t * self, unsigned int numstate)
{
    self->initial_state_for_tracing = numstate;
}

unsigned int tracer_howManyStatesCurrently(tracer_t * self)
{
    return self->graph->nb_states;
}

int filter_filtre_depth(filter_t * filter, state_t * state)
{
    if(filter->depth == 0)
        return FILTER_OK;
    return filter_limit_on_depth(state, filter->depth);
}

int filter_filtre_diff_depth(filter_t * filter, state_t * state)
{
    if(filter->diff_depth == 0)
        return FILTER_OK;
    return filter_limit_on_depth(state, filter->tracer->graph->list_of_states[filter->tracer->initial_state_for_tracing]->depth + filter->diff_depth );
}


int filter_filtre_max_queue(filter_t * filter, state_t * state)
{
    if(filter->nb_max_msg == 0)
        return FILTER_OK;
    return filter_limit_on_max_nb_msg_in_oriented_bgp_session(state, filter->nb_max_msg);
}

int filter_filter_withdrawMsg(filter_t * filter, state_t * state)
{
   if(filter->rejectWithdrawMsg == FALSE)
       return FILTER_OK;

   if(state_has_withdrawMsg(state) == 0)
       return FILTER_OK;
   else
       return FILTER_NOK;
}


int filter_filter_multipleMsgInSameSession(filter_t * filter, state_t * state)
{
   if(filter->rejectmultipleMsgInSameSession == FALSE)
       return FILTER_OK;

   if ( state_multiple_msg_in_oriented_bgp_session(state) == 0 )
       return FILTER_OK;
   else
       return FILTER_NOK;

}


void filter_set_maxNbOfTreatedTransitions(tracer_t * tracer, unsigned int max)
{
    tracer->filter->maxNbOfTreatedTransitions = max;
}

void filter_unset_maxNbOfTreatedTransitions(tracer_t * tracer)
{
    tracer->filter->maxNbOfTreatedTransitions = 0;
}

void filter_withdrawMsg_accept(tracer_t * tracer)
{
    tracer->filter->rejectWithdrawMsg = FALSE;
}

void filter_withdrawMsg_reject(tracer_t * tracer)
{
    tracer->filter->rejectWithdrawMsg = TRUE;
}


void filter_multipleMsgInSameSession_accept(tracer_t * tracer)
{
    tracer->filter->rejectmultipleMsgInSameSession = FALSE;
}

void filter_multipleMsgInSameSession_reject(tracer_t * tracer)
{
    tracer->filter->rejectmultipleMsgInSameSession = TRUE;
}

int filter_filtre_maxNbOfTreatedTransitions(filter_t * filter, unsigned int nboftrans)
{
    if(filter->maxNbOfTreatedTransitions == 0 ||
            nboftrans < filter->maxNbOfTreatedTransitions )
        return FILTER_OK;
    else
        return FILTER_NOK;

}

int filter_apply_on_state(filter_t * filter, state_t * state )
{
    if ( state == NULL )
        return FILTER_NOK;
    if ( filter_filtre_depth(filter, state) == FILTER_NOK )
        return FILTER_NOK;        
    if ( filter_filtre_diff_depth(filter, state) == FILTER_NOK )
        return FILTER_NOK;
    if ( filter_filtre_max_queue(filter, state) == FILTER_NOK )
        return FILTER_NOK;
    if ( filter_filter_withdrawMsg(filter, state) == FILTER_NOK )
        return FILTER_NOK;
    if ( filter_filter_multipleMsgInSameSession(filter, state) == FILTER_NOK )
        return FILTER_NOK;


    return FILTER_OK;
}

void tracer_scheduler_set(tracer_t * self, tracing_scheduler_type_t type)
{
    assert(type < TRACING_SCHEDULER_MAX );
    self->tracing_scheduler_type = type;
}

static unsigned int MAXFIFODEPTH = 10000;

tracing_scheduler_t * tracing_scheduler_1_fifo_create()
{
   tracing_scheduler_1_fifo_t * sched =
    (tracing_scheduler_1_fifo_t *) MALLOC(sizeof(tracing_scheduler_1_fifo_t));
  sched->header.type= TRACING_SCHEDULER_1_FIFO;
  
  sched->list_of_state_trans = fifo_create(MAXFIFODEPTH, destroy_struct_state_trans);
        
   
  return (tracing_scheduler_t *) sched;
}

tracing_scheduler_t * tracing_scheduler_2_lifo_create()
{
   tracing_scheduler_2_lifo_t * sched =
    (tracing_scheduler_2_lifo_t *) MALLOC(sizeof(tracing_scheduler_2_lifo_t));
  sched->header.type= TRACING_SCHEDULER_2_LIFO;
  
  sched->list_of_state_trans = lifo_create(MAXFIFODEPTH, destroy_struct_state_trans);  
  
  
  return (tracing_scheduler_t *) sched;
}

tracing_scheduler_t * tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_create(tracer_t * tracer)
{
   tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t * sched =
    (tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t *) MALLOC(sizeof(tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t));
  sched->header.type= TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION;
  
  // choses propres à ce scheduler : 
  sched->current_working_state = NULL;
  sched->next_trans = 0;
  sched->started = FALSE;
  
  sched->nb_states = 0;
  sched->max_states = MAX_STATE;
  sched->tracer = tracer;
  sched->list_of_states = (state_t **) MALLOC(sched->max_states * sizeof(state_t *));
  sched->change_state = TRUE;
  
  return (tracing_scheduler_t *) sched;
}

tracing_scheduler_t * tracing_scheduler_create(tracing_scheduler_type_t type, tracer_t * tracer )
{
    assert(type < TRACING_SCHEDULER_MAX);

  switch (type) {
  case TRACING_SCHEDULER_1_FIFO:
    return  tracing_scheduler_1_fifo_create();
    break;
  case TRACING_SCHEDULER_2_LIFO:
    return  tracing_scheduler_2_lifo_create();
    break;
  case TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION:
    return  tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_create(tracer);
    break;
  case TRACING_SCHEDULER_4:
    return NULL;
    break;
  default:
    printf("should never reach this code!!!! \n");
    return NULL;
}
}


int tracing_scheduler_1_fifo_empty(tracing_scheduler_1_fifo_t * sched )
{
    if( fifo_depth(sched->list_of_state_trans)==0 )
        return TRUE;
    else return FALSE;
}

int tracing_scheduler_2_lifo_empty(tracing_scheduler_2_lifo_t * sched )
{
    if( lifo_depth(sched->list_of_state_trans)==0 )
        return TRUE;
    else return FALSE;
}

int tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_empty(tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t * sched )
{
    if(sched->current_working_state == NULL ||
         (
          sched->change_state == TRUE && 
          NULL == get_state_with_mininum_bigger_number_of_msg_in_session(sched->tracer->graph->nb_states, sched->tracer->graph->list_of_states)            
         )
       )
       return TRUE;        
    else return FALSE;
}
int tracing_scheduler_4_empty(tracing_scheduler_4_t * sched )
{
    return TRUE;
}

void tracing_scheduler_1_fifo_set_empty(tracing_scheduler_1_fifo_t * sched )
{
    fifo_empty(sched->list_of_state_trans);
}

void tracing_scheduler_2_lifo_set_empty(tracing_scheduler_2_lifo_t * sched )
{
    lifo_empty(sched->list_of_state_trans);
}

void tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_set_empty(tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t * sched )
{
   sched->current_working_state = NULL;
   // a compléter
   FREE(sched->list_of_states);
}
void tracing_scheduler_4_set_empty(tracing_scheduler_4_t * sched )
{
    //
}

int tracing_scheduler_empty(tracing_scheduler_t * sched)
{
  assert(sched->type < TRACING_SCHEDULER_MAX);

  switch (sched->type) {
  case TRACING_SCHEDULER_1_FIFO:
    return tracing_scheduler_1_fifo_empty((tracing_scheduler_1_fifo_t *) sched);
    break;
  case TRACING_SCHEDULER_2_LIFO:
    return tracing_scheduler_2_lifo_empty((tracing_scheduler_2_lifo_t *) sched);
    break;
  case TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION:
    return tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_empty((tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t *) sched);
    break;
  case TRACING_SCHEDULER_4:
    return tracing_scheduler_4_empty((tracing_scheduler_4_t *) sched);
    break;
  default:
    printf("should never reach this code!!!! \n");
    return TRACING_SCHEDULER_MAX;
  }
}


void tracing_scheduler_set_empty(tracing_scheduler_t * sched)
{
  assert(sched->type < TRACING_SCHEDULER_MAX);

  switch (sched->type) {
  case TRACING_SCHEDULER_1_FIFO:
     tracing_scheduler_1_fifo_set_empty((tracing_scheduler_1_fifo_t *) sched);
    break;
  case TRACING_SCHEDULER_2_LIFO:
     tracing_scheduler_2_lifo_set_empty((tracing_scheduler_2_lifo_t *) sched);
    break;
  case TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION:
     tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_set_empty((tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t *) sched);
    break;
  case TRACING_SCHEDULER_4:
     tracing_scheduler_4_set_empty((tracing_scheduler_4_t *) sched);
    break;
  default:
    printf("should never reach this code!!!! \n");
    
  }
}


void tracing_scheduler_1_fifo_push(tracing_scheduler_1_fifo_t * sched, state_trans_t * state_trans)
{
    fifo_push(sched->list_of_state_trans, state_trans);
}
void tracing_scheduler_2_lifo_push(tracing_scheduler_2_lifo_t * sched, state_trans_t * state_trans)
{    //printf("LIFO_push\n");

    lifo_push(sched->list_of_state_trans, state_trans);
}

void tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_push(tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t * sched, state_trans_t * state_trans)
{
    // ce qui nous intéresse ce sont les states qu'on visite, (pas les transitions associées)
    // donc on ajoute l'état s'il n'est pas encore dans notre liste d'états.
    int i=0;
    while( i < sched->nb_states )
    {
        if(sched->list_of_states[i] == sched->tracer->graph->list_of_states[state_trans->state])
        {
            // si présent, ne rien faire ...
            // s'arrêter !
            return;
        }
        i++;
    }
    // on a tout parcouru, et il n'est pas là 
    sched->list_of_states[sched->nb_states] = sched->tracer->graph->list_of_states[state_trans->state];
    sched->nb_states ++;
    printf("SCHED-PUSH, on ajoute state %u\n", state_trans->state );
    
    if(sched->started == FALSE)
    {
         sched->current_working_state = get_state_with_mininum_bigger_number_of_msg_in_session(sched->nb_states, sched->list_of_states);
         sched->next_trans = 0;
    }
        
    //lifo_push(sched->list_of_state_trans, state_trans);
    
}
void tracing_scheduler_4_push(tracing_scheduler_4_t * sched, state_trans_t * state_trans)
{
    //lifo_push(sched->list_of_state_trans, state_trans);
}

void tracing_scheduler_push(tracing_scheduler_t * sched, state_trans_t * state_trans)
{
  assert(sched->type < TRACING_SCHEDULER_MAX);

  switch (sched->type) {
  case TRACING_SCHEDULER_1_FIFO:
       tracing_scheduler_1_fifo_push((tracing_scheduler_1_fifo_t *) sched, state_trans);
    break;
  case TRACING_SCHEDULER_2_LIFO:
       tracing_scheduler_2_lifo_push((tracing_scheduler_2_lifo_t *) sched, state_trans);
    break;
  case TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION:
       tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_push((tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t *) sched, state_trans);
    break;
  case TRACING_SCHEDULER_4:
       tracing_scheduler_4_push((tracing_scheduler_4_t *) sched, state_trans);
    break;
  default:
    printf("should never reach this code!!!! \n");
    
  }
}

state_trans_t *  tracing_scheduler_1_fifo_pop(tracing_scheduler_1_fifo_t * sched)
{
    return (state_trans_t *) fifo_pop(sched->list_of_state_trans);
}

state_trans_t *  tracing_scheduler_2_lifo_pop(tracing_scheduler_2_lifo_t * sched)
{
    //printf("LIFO_pop\n");
    return (state_trans_t *) lifo_pop(sched->list_of_state_trans);
}

state_trans_t *  tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_pop(tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t * sched)
{
    if(sched->started == FALSE)
        sched->started = TRUE;
    
    if(sched->change_state == TRUE)
    {
         sched->current_working_state = get_state_with_mininum_bigger_number_of_msg_in_session(sched->tracer->graph->nb_states, sched->tracer->graph->list_of_states);
         sched->next_trans = 0;
         sched->change_state = FALSE;
    }
    
    if(sched->current_working_state == NULL)
    {
        printf("SCHED-POP : plus de state !\n");
        return NULL;        
    }
    else
    {
        state_trans_t * state_trans = ( state_trans_t *) MALLOC ( sizeof( state_trans_t));
        state_trans->state= sched->current_working_state->id;
        state_trans->trans= sched->next_trans;        
        
        if(sched->next_trans < state_calculate_allowed_output_transitions(sched->current_working_state) -1 )
        {
            sched->next_trans ++ ;
        }
        else
        {
             // il faut passer au state suivant, on a traité toutes les transitions !
            sched->change_state = TRUE;
        }
        printf("SCHED-POP : voici la transition a traiter : %u(%u) !\n",state_trans->state,state_trans->trans);
        return state_trans;
    }
}

state_trans_t *  tracing_scheduler_4_pop(tracing_scheduler_4_t * sched)
{
    return NULL;//(state_trans_t *) lifo_pop(sched->list_of_state_trans);
}

state_trans_t * tracing_scheduler_pop(tracing_scheduler_t * sched)
{
  assert(sched->type < TRACING_SCHEDULER_MAX);

  switch (sched->type) {
  case TRACING_SCHEDULER_1_FIFO:
    return tracing_scheduler_1_fifo_pop((tracing_scheduler_1_fifo_t *) sched);
    break;
  case TRACING_SCHEDULER_2_LIFO:
    return tracing_scheduler_2_lifo_pop((tracing_scheduler_2_lifo_t *) sched);
    break;
  case TRACING_SCHEDULER_3_MIN_MAX_NBMSGBGPSESSION:
    return tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_pop((tracing_scheduler_3_MIN_MAX_NBMSGBGPSESSION_t *) sched);
    break;
  case TRACING_SCHEDULER_4:
    return tracing_scheduler_4_pop((tracing_scheduler_4_t *) sched);
    break;
  default:
    printf("should never reach this code!!!! \n");
    return NULL;
  }
}



int tracer_trace_whole_graph(tracer_t * self)
{
    return tracer_trace_whole_graph___GENERIC(self);
    //return  tracer_trace_whole_graph_v1bis(self);
}

int tracer_trace_whole_graph___GENERIC(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int treated_transitions = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    
    tracing_scheduler_t * scheduler = tracing_scheduler_create(self->tracing_scheduler_type, self);
    
    state_trans_t * state_trans;

    _tracer_start(self);

    //filter_set_maxNbOfTreatedTransitions(self, 200);

    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;        
        tracing_scheduler_push(scheduler , state_trans);
        
    }
//  fin de l'amorce


    while(tracing_scheduler_empty(scheduler) == FALSE )
    {
        state_trans =  tracing_scheduler_pop(scheduler);
        
        if( filter_filtre_maxNbOfTreatedTransitions(self->filter, treated_transitions ) == FILTER_OK ) 
        {
            treated_transitions++;
            printf("Generated transition : %u    %u(%u)\n",treated_transitions, state_trans->state, state_trans->trans);

            tracer_trace_from_state_using_transition(self, state_trans->state,state_trans->trans);
            //printf("2: after tracing");
            // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
            // c'est un nouvel état si
            // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
            // si cet état a une seule input_transition alors c'est un nouveau.
            current_state = self->graph->list_of_states[state_trans->state];

            // trouver la transition qui porte le numéro state_trans->trans
            for( i=0 ; i < current_state->nb_output ; i++) // -1; i>=0 ; i--)
            {
                if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                    break;
            }
            assert(i < current_state->nb_output);

            if( current_state->output_transitions[i]->to->nb_input == 1)
            {
                current_state = current_state->output_transitions[i]->to;
                current_state_id = current_state->id;
                nbtrans = state_calculate_allowed_output_transitions(current_state);

                /*if(  filter_filtre_depth(self, current_state) == FILTER_OK
                  && filter_filtre_max_queue(self, current_state) == FILTER_OK
                  && filter_filtre_maxNbOfTreatedTransitions(self, treated_transitions ) == FILTER_OK )
                 * */
                if ( filter_apply_on_state( self->filter, current_state))        
                {
                  for( i = 0 ; i < nbtrans ; i++ )
                  {
                    state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                    state_trans->state=current_state_id;
                    state_trans->trans=i;
                    tracing_scheduler_push(scheduler , state_trans);
                  }
                }
            }
        }
        else
        {
            tracing_scheduler_set_empty(scheduler);
        }
    }
    return self->graph->nb_states;
}



int tracer_trace_auto(tracer_t * self)
{
    if(self->started == 0)
        return -1;
// le tracer doit etre démarré !!
    
    unsigned int treated_transitions = 0;
    unsigned int nbtrans;
    unsigned int current_state_id = 0;
    unsigned int i;
    state_t * current_state;
    
    tracing_scheduler_t * scheduler = tracing_scheduler_create(self->tracing_scheduler_type, self);
    
    state_trans_t * state_trans;

    //_tracer_start(self);
    //filter_set_maxNbOfTreatedTransitions(self, 200);
    
    

    //amorce :
    // ajouter toutes les transitions dispo de l'état "initial" dans le scheduler
    //current_state_id=0;
    current_state = self->graph->list_of_states[self->initial_state_for_tracing];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=self->initial_state_for_tracing;
        state_trans->trans=i;        
        tracing_scheduler_push(scheduler , state_trans);        
    }
//  fin de l'amorce


    while(tracing_scheduler_empty(scheduler) == FALSE )
    {
        state_trans =  tracing_scheduler_pop(scheduler);
        
        if( filter_filtre_maxNbOfTreatedTransitions(self->filter, treated_transitions ) == FILTER_OK ) 
        {
            treated_transitions++;
            printf("Generated transition : %u    %u(%u)\n",treated_transitions, state_trans->state, state_trans->trans);

            tracer_trace_from_state_using_transition(self, state_trans->state,state_trans->trans);
            //printf("2: after tracing");
            // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
            // c'est un nouvel état si
            // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
            // si cet état a une seule input_transition alors c'est un nouveau.
            current_state = self->graph->list_of_states[state_trans->state];

            // trouver la transition qui porte le numéro state_trans->trans
            for( i=0 ; i < current_state->nb_output ; i++) // -1; i>=0 ; i--)
            {
                if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                    break;
            }
            assert(i < current_state->nb_output);

            if( current_state->output_transitions[i]->to->nb_input == 1)
            {
                current_state = current_state->output_transitions[i]->to;
                current_state_id = current_state->id;
                nbtrans = state_calculate_allowed_output_transitions(current_state);

                /*if(  filter_filtre_depth(self, current_state) == FILTER_OK
                  && filter_filtre_diff_depth(self, current_state) == FILTER_OK
                  && filter_filtre_max_queue(self, current_state) == FILTER_OK
                  && filter_filtre_maxNbOfTreatedTransitions(self, treated_transitions ) == FILTER_OK )*/
                if ( filter_apply_on_state(self->filter, current_state))
                {
                  for( i = 0 ; i < nbtrans ; i++ )
                  {
                    state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                    state_trans->state=current_state_id;
                    state_trans->trans=i;
                    tracing_scheduler_push(scheduler , state_trans);
                  }
                }
            }
        }
        else
        {
            tracing_scheduler_set_empty(scheduler);
        }
    }
    return self->graph->nb_states;
}




/*/
// FIFO ==> une file ==> parcours en largeur d'abord
int tracer_trace_whole_graph_v0_FIFO(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int treated_transitions = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    gds_fifo_t * list_of_state_trans;

    state_trans_t * state_trans;

    _tracer_start(self);

    list_of_state_trans = fifo_create(MAXFIFODEPTH, destroy_struct_state_trans);


    filter_set_maxNbOfTreatedTransitions(self, 200);

    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;
        fifo_push(list_of_state_trans, state_trans);
    }
//  fin de l'amorce


    while(fifo_depth(list_of_state_trans)!=0)
    {
        state_trans = (state_trans_t *) fifo_pop(list_of_state_trans);
        treated_transitions++;
        printf("Generated transition : %u\n",treated_transitions);

        tracer_trace_from_state_using_transition(self, state_trans->state,state_trans->trans);
        //printf("2: after tracing");
        // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
        // c'est un nouvel état si
        // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
        // si cet état a une seule input_transition alors c'est un nouveau.
        current_state = self->graph->list_of_states[state_trans->state];
        // trouver la transition qui porte le numéro state_trans->trans
        for(i=current_state->nb_output-1; i>=0; i--)
        {
            if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                break;
        }
        assert(i>=0);
        if( current_state->output_transitions[i]->to->nb_input == 1)
        {
            current_state = current_state->output_transitions[i]->to;
            current_state_id = current_state->id;
            nbtrans = state_calculate_allowed_output_transitions(current_state);

            if(  filter_filtre_depth(self, current_state) == FILTER_OK
              && filter_filtre_max_queue(self, current_state) == FILTER_OK
              && filter_filtre_maxNbOfTreatedTransitions(self, treated_transitions ) == FILTER_OK )
            {
              for( i = 0 ; i < nbtrans ; i++ )
              {
                state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                state_trans->state=current_state_id;
                state_trans->trans=i;
                fifo_push(list_of_state_trans, state_trans);
              }
            }
        }
    }
    return self->graph->nb_states;
}
//*/

/*/
// same as v0 but use a lifo instead of a fifo
// should be refactored with the v0 (because it's just a copy/paste)
// LIFO ==> une pile ==> parcours en profondeur d'abord

int tracer_trace_whole_graph_v1_LIFO(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int treated_transitions = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    gds_lifo_t * list_of_state_trans;

    state_trans_t * state_trans;

    _tracer_start(self);

    list_of_state_trans = lifo_create(MAXFIFODEPTH, destroy_struct_state_trans);

    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;
        lifo_push(list_of_state_trans, state_trans);
    }
//  fin de l'amorce


    while(lifo_depth(list_of_state_trans)!=0)
    {
        state_trans = (state_trans_t *) lifo_pop(list_of_state_trans);
        treated_transitions++;
        printf("Generated transition : %u\n",treated_transitions);
       // if(work>18)
       //         tracer_graph_export_dot(gdsout,self);
if(treated_transitions==200)
{
return;
}
        //printf("2: before tracing");
        tracer_trace_from_state_using_transition(self, state_trans->state,state_trans->trans);
        //printf("2: after tracing");
        // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
        // c'est un nouvel état si
        // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
        // si cet état a une seule input_transition alors c'est un nouveau.
        current_state = self->graph->list_of_states[state_trans->state];
        // trouver la transition qui porte le numéro state_trans->trans
        for(i=current_state->nb_output-1; i>=0; i--)
        {
            if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                break;
        }
        assert(i>=0);
        if( current_state->output_transitions[i]->to->nb_input == 1)
        {
            current_state = current_state->output_transitions[i]->to;
            current_state_id = current_state->id;
            nbtrans = state_calculate_allowed_output_transitions(current_state);

            if(  filter_filtre_depth(self, current_state) == FILTER_OK
              && filter_filtre_max_queue(self, current_state) == FILTER_OK  )
            {
              for( i = 0 ; i < nbtrans ; i++ )
              {
                state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                state_trans->state=current_state_id;
                state_trans->trans=i;
                lifo_push(list_of_state_trans, state_trans);
              }
            }
        }
    }
    return self->graph->nb_states;
}
//*/

/*/

// same as v1 but with a max graph depth.
// should be refactored with the v1
// LIFO ==> une pile ==> parcours en profondeur d'abord
int tracer_trace_whole_graph_v1bis_LIFO_AND_MAX_GRAPH_DEPTH(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    unsigned int treated_transitions = 0;
    unsigned int nbtrans;
    unsigned int current_state_id;
    unsigned int i;
    state_t * current_state;
    gds_lifo_t * list_of_state_trans;

    struct state_trans_t * state_trans;

    _tracer_start(self);

    list_of_state_trans = lifo_create(MAXFIFODEPTH, destroy_struct_state_trans);
    
    //amorce :
    // ajouter toutes les transitions dispo de l'état 0 dans la fifo
    current_state_id=0;
    current_state = self->graph->list_of_states[current_state_id];
    nbtrans = state_calculate_allowed_output_transitions(current_state);

    for( i = 0 ; i < nbtrans ; i++ )
    {
        state_trans = (struct state_trans_t *) MALLOC ( sizeof(struct state_trans_t));
        state_trans->state=current_state_id;
        state_trans->trans=i;
        lifo_push(list_of_state_trans, state_trans);
    }
//  fin de l'amorce
    
    //unsigned int MAX_GRAPH_DEPTH = 11;
    while(lifo_depth(list_of_state_trans)!=0)
    {   
        state_trans = (struct state_trans_t *) lifo_pop(list_of_state_trans);
        treated_transitions++;
        printf("Treated transitions : %u, From state %u at depth %u using transition nb %u\n",treated_transitions, state_trans->state,  self->graph->list_of_states[state_trans->state]->depth, state_trans->trans);
       // if(work>18)
       //         tracer_graph_export_dot(gdsout,self);
        if(treated_transitions==1000)
        {
                return;
        }
        //printf("2: before tracing :  state %u , trans %u\n",state_trans->state ,state_trans->trans);
        tracer_trace_from_state_using_transition(self, state_trans->state, state_trans->trans);
        //printf("2: after tracing\n");
        // si nouvel état créé, on l'ajoute dans la file, sinon on passe!
        // c'est un nouvel état si
        // a partir de l'état créé, on prend la transition créée, on prend l'état au bout de la transition
        // si cet état a une seule input_transition alors c'est un nouveau.
        //printf("ici\n");//,current_state->depth);
        current_state = self->graph->list_of_states[state_trans->state];
        // trouver la transition qui porte le numéro state_trans->trans
        for(i=current_state->nb_output-1; i>=0; i--)
        {
            if(current_state->output_transitions[i]->num_trans==state_trans->trans)
                break;
        }
        assert(i>=0);
        if( current_state->output_transitions[i]->to->nb_input == 1)
        {
            current_state = current_state->output_transitions[i]->to;
            current_state_id = current_state->id;
            nbtrans = state_calculate_allowed_output_transitions(current_state);
            //printf("new state : %u - depth :%u\n",current_state->id, current_state->depth);

            if(  filter_filtre_depth(self, current_state) == FILTER_OK
              && filter_filtre_max_queue(self, current_state) == FILTER_OK  )
                        {
              for( i = 0 ; i < nbtrans ; i++ )
              {
                state_trans = (state_trans_t *) MALLOC ( sizeof(state_trans_t));
                state_trans->state=current_state_id;
                state_trans->trans=i;
                lifo_push(list_of_state_trans, state_trans);
              }
            }
        }
    }
  
    return self->graph->nb_states;
}
//*/

/*/
int tracer_trace_whole_graph_v2_TYPE3_MINBGPSESSION(tracer_t * self)
{
    if(self->started == 1)
        return -1;

    _tracer_start(self);

    unsigned int nbtrans;
    unsigned int i;
    unsigned int treated_transitions = 0;
    state_t * min_state;

    unsigned int nb_trans_limit = 1000;
    while( treated_transitions < nb_trans_limit
            && (min_state = get_state_with_mininum_bigger_number_of_msg_in_session(self->graph->nb_states, self->graph->list_of_states))
             != NULL )
    {
        //printf("Min_state = %u \n",min_state->id);

        // traiter toutes ses transitions.
        // ici quand on prend un état, on traite toutes ses transitions !
        nbtrans = state_calculate_allowed_output_transitions(min_state);

        for( i = 0 ; i < nbtrans ; i++)
        {
           // printf("\t Transition num %u \n",i);

            tracer_trace_from_state_using_transition(self,min_state->id,i);
            treated_transitions++;
        }
    }
    printf("nb transition : %u \n",treated_transitions);
    return self->graph->nb_states;
}
//*/


int tracer_trace_whole_branch_from_state(tracer_t * self, unsigned int state_id)
{
    if(self->started == 0)
        return -1;


}


int tracer_trace_from_state_using_transition(tracer_t * self, unsigned int state_id, unsigned int transition_id )
{
    state_t * origin_state;

    int debug = 0;
    
    if(self->started == 0)
        return -1;

    //chercher l'état,
    //l'injecter dans cbgp
    //choisir de simuler l'événement num_event
    //créer la transition
    //prendre copie de l'état courant de cbgp
    //attacher cet état correctement dans le graphe.

    if(state_id >= graph_max_num_state())
    {
        if(debug==1) printf("ATTENTION ! nombre max d'état = %d\n",graph_max_num_state());
        return 1;
    }
    assert(self->graph->list_of_states != NULL );
    origin_state = self->graph->list_of_states[state_id];
    if(origin_state == NULL)
    {
        if(debug==1) printf("Unexisting state !\n");
        return 1;
    }

    state_inject(origin_state);

    transition_t * transition = state_generate_transition(origin_state,transition_id);
    // TO DO TODO
    //vérifier que la transition n'a pas déjà été visitée !
    // transition->to == NULL
    if(transition == NULL) // actuellement si transition déjà générée --> return NULL
        return 10;

    unsigned int num_event =  origin_state->allowed_output_transitions[transition_id];

    //let the num_event be the next event to run
    if(debug==1) printf("2: tracing : before setfirst\n");
    tracer_get_simulator(self)->sched->ops.set_first(tracer_get_simulator(self)->sched, num_event );
    if(debug==1) printf("2: tracing : before sim step 1\n");

    sim_step(tracer_get_simulator(self), 1);
    if(debug==1) printf("2: tracing : before create state\n");

    state_t * new_state;
    if( STATE_STORAGE_METHOD == VERSION_1 )
        new_state = state_create_isolated(self);
    else if ( STATE_STORAGE_METHOD == VERSION_2 )
        new_state = state_create_isolated_from_ST_TR(self, self->graph->list_of_states[state_id] , transition );

    // vérifier si l'état n'est pas déjà présent
     if(debug==1) printf("2: tracing : before check identical state\n");

    state_t * identical_state = graph_search_identical_state(self->graph, new_state);
     if(debug==1) printf("2: tracing : after check if there is an identical state\n");

    if( identical_state == NULL )
    {
        // attacher l'état au graphe  (et valider le numéro state-id)
        if(debug==1) printf("2: tracing : before attach state to graph\n");

        state_attach_to_graph(new_state, transition);
                if(debug==1) printf("2: tracing : after attach state to graph\n");
    }
    else
    {
                if(debug==1) printf("2: tracing : before add input transition\n");

        state_add_input_transition(identical_state,transition);
                if(debug==1) printf("2: tracing : after add input trans\n");

        // récupérer l'état identique
        // libérer l'état nouvellement créé :
        // free(state) ???
        // ajouter une transition entrante
        // ne pas valider le numéro state-id
        FREE(new_state);

        // vérifier si un cycle a été créé :
        cycle_t * a_cycle = NULL ;
        a_cycle = graph_detect_one_cycle(self->graph);
        if(a_cycle != NULL)
        {
            printf("\nCycle detected : \n[ ");
            int j=0;
            for(j = 0 ; j < a_cycle->nodes_cycles_length ; j++ )
            {
                printf("%u -> ", a_cycle->nodes_cycles[j]);
            }
            printf("%u ] \n", a_cycle->nodes_cycles[0]);
        }

    }

     if(debug==1) printf("2: tracing : end of tracing\n");



    return 0;
}

   void tracer_graph_detect_every_cycle(gds_stream_t * stream,tracer_t * tracer)
   {
       graph_detect_every_cycle(tracer->graph);
       graph_cycle_dump(stream,tracer->graph);
   }

   void tracer_graph_cycle_dump(gds_stream_t * stream,tracer_t * tracer)
   {
       graph_cycle_dump(stream,tracer->graph);
   }
   void tracer_graph_final_state_dump(gds_stream_t * stream,tracer_t * tracer)
   {
       graph_final_state_dump(stream,tracer->graph);
   }


int _tracer_dump(gds_stream_t * stream, tracer_t * tracer)
{
    if(tracer->started == 0)
    {
        stream_printf(stream,"Tracer not yet started\n");        
        return 0;
    }
    else
    {
        stream_printf(stream,"Tracer Ready : \n");
        return state_dump(stream,tracer->graph->state_root);
        //return 1;
    }
}

/*
int FOR_TESTING_PURPOSE_tracer_graph_state_dump(gds_stream_t * stream, tracer_t * self , unsigned int num_state)
{
    return FOR_TESTING_PURPOSE_graph_state_dump(stream,self->graph,num_state);
}*/

/*int FOR_TESTING_PURPOSE_tracer_graph_current_state_dump(gds_stream_t * stream, tracer_t * self)
{
    return FOR_TESTING_PURPOSE_graph_current_state_dump(stream,self->graph);
}
*/

   int tracer_graph_allstates_dump(gds_stream_t * stream,tracer_t *  tracer)
   {
          return graph_allstates_dump(stream,tracer->graph);
   }

/*
int FOR_TESTING_PURPOSE_tracer_inject_state(tracer_t * tracer , unsigned int num_state)
{
    return FOR_TESTING_PURPOSE_graph_inject_state(tracer->graph , num_state);
}
 * */

sched_tunable_t * tracer_get_tunable_scheduler(tracer_t * tracer)
{
    return (sched_tunable_t *) tracer->network->sim->sched;
}

simulator_t * tracer_get_simulator(tracer_t * tracer)
{
    return tracer->network->sim;
}

  int tracer_inject_state(tracer_t * tracer , unsigned int num_state)
  {
      return graph_inject_state(tracer->graph , num_state);
  }

   int tracer_state_dump(gds_stream_t * stream, tracer_t * tracer , unsigned int num_state)
   {
      return graph_state_dump(stream, tracer->graph , num_state);

   }

   void tracer_graph_export_dot(gds_stream_t * stream,tracer_t * tracer)
   {
       graph_export_dot_to_stream(stream,tracer->graph);
   }



/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _network_init ]--------------------------------------------

   
   filter_t * tracer_filter_create(tracer_t * tracer)
   {
        filter_t * filter;
        filter=(filter_t *) MALLOC(sizeof(filter_t));
        
        filter->tracer = tracer;
        
        filter->depth=0;
        filter->nb_max_msg=0;
        filter->maxNbOfTreatedTransitions = 0;
        filter->diff_depth=0;
        filter->rejectWithdrawMsg = FALSE;
        filter->rejectmultipleMsgInSameSession = FALSE;
        return filter;
   }
   
   
// concider the simulator current state as the initial state.


tracer_t * tracer_create(network_t * network)
{
  tracer_t * tracer;
  tracer=(tracer_t *) MALLOC(sizeof(tracer_t));
  tracer->network = network;
  tracer->graph = graph_create(tracer);
  tracer->started = 0;
  tracer->nodes = NULL;
  tracer->nb_nodes = 0;
  
  tracer->tracing_scheduler = NULL;
  //default : 
  tracer->tracing_scheduler_type = TRACING_SCHEDULER_2_LIFO;
  tracer->initial_state_for_tracing = 0;
  tracer->filter = tracer_filter_create(tracer);
          
          //(net_node_t **) MALLOC(sizeof(net_node_t *) * trie_num_nodes(network->nodes, 1));

  time_t curtime;
  struct tm * loctime;
  /* Get the current time.  */
  curtime = time (NULL);
  /* Convert it to local time representation.  */
  loctime = localtime (&curtime);
  //tracer->base_output_file_name = (char *) MALLOC ( 256 * sizeof(char));

  sprintf(tracer->base_output_directory,"/home/yo/tmp/tracer_cbgp/%d_%d_%d_%d:%d:%d/",
      loctime->tm_year,loctime->tm_mon,loctime->tm_mday,loctime->tm_hour,loctime->tm_min,loctime->tm_sec);
  
  printf("OUTPUT DIRECTORY : \n/home/yo/tmp/tracer_cbgp/%d_%d_%d_%d:%d:%d/\n",
      loctime->tm_year,loctime->tm_mon,loctime->tm_mday,loctime->tm_hour,loctime->tm_min,loctime->tm_sec);
  
  char commande[1024];
  sprintf(commande,"mkdir %s",tracer->base_output_directory);
  system(commande);

  sprintf(tracer->base_output_file_name,"tracer_");
  sprintf(tracer->IMAGE_FORMAT,IMAGE_GRAPHVIZ_FORMAT);
  
  return tracer;
}

tracer_t * _tracer_create()
{
  return tracer_create( network_get_default());
}

tracer_t * tracer_get_default()
{
  assert(_default_tracer != NULL);
  return _default_tracer;
}


void _tracer_init()
{
  _default_tracer= _tracer_create();
}

int _tracer_start(tracer_t * self)
{
     if(self->started == 1)
         return 0;

     self->nodes = (net_node_t **) _trie_get_array(self->network->nodes)->data ;
     self->nb_nodes = trie_num_nodes(self->network->nodes, 1);

     graph_init(self->graph);

     //self->graph->state_root->graph=self->graph;
     // the first state doesn't have the graph.
     
     self->started = 1;
     return 1;
}



/*
// -----[ _network_done ]--------------------------------------------
void _tracer_done()
{
  if (_default_tracer != NULL)
    tracer_destroy(&_default_tracer);
}
*/


void tracer_graph_export_allStates_to_file(tracer_t * tracer)
{
    graph_export_allStates_to_file(tracer->graph);
}

void tracer_graph_export_dot_to_file(tracer_t * tracer)
{
    graph_export_dot_to_file(tracer->graph);
}

    void tracer_graph_export_dot_allStates_to_file_OLD(tracer_t * tracer)
    {
         graph_export_allStates_dot_to_file_OLD(tracer->graph);
    }
    
    void tracer_graph_export_allStates_loc_rib_dot(tracer_t * tracer)
    {
        graph_export_allStates_loc_rib_dot(tracer->graph);
    }