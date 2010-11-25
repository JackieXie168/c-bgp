/* 
 * File:   state.h
 * Author: yo
 *
 * Created on 25 novembre 2010, 11:48
 */

#ifndef __STATE_H__
#define	__STATE_H__



// -----[ sched_type_t ]---------------------------------------------
/** Possible scheduling techniques. */
typedef enum {
  SCHEDULER_STATIC,
  SCHEDULER_DYNAMIC,
  SCHEDULER_TUNABLE,
  SCHEDULER_MAX
} sched_type_t;


// -----[ sim_event_ops_t ]------------------------------------------
/** Virtual methods of simulation events. */
typedef struct {
  int  (*callback) (struct simulator_t * sim, void * ctx);
  void (*destroy) (void * ctx);
  void (*dump)(gds_stream_t * stream, void * ctx);
} sim_event_ops_t;


// -----[ sched_ops_t ]----------------------------------------------
/** Virtual methods of schedulers. */
typedef struct sched_ops_t {
  void         (*destroy) (struct sched_t ** self_ref);
  void         (*cancel) (struct sched_t * self);
  void         (*clear) (struct sched_t * self);
  net_error_t  (*run) (struct sched_t * self, unsigned int num_steps);
  int          (*post) (struct sched_t * self, const sim_event_ops_t * ops,
			void * ctx, double time, sim_time_t time_type);
  unsigned int (*num_events) (struct sched_t * self);
  void *       (*event_at) (struct sched_t * self, unsigned int index);
  void         (*dump_events) (gds_stream_t * stream, struct sched_t * self);
  void         (*set_log_process) (struct sched_t * self,
				   const char * file_name);
  double       (*cur_time) (struct sched_t * self);
  int          (*set_first) (struct sched_t * self, unsigned int nb);
  int          (*swap) (struct sched_t * self, unsigned int nb1, unsigned int nb2);
  int          (*bringForward) (struct sched_t * self, unsigned int num);
  double       (*runOpenSessions) (struct sched_t * self);
} sched_ops_t;





// -----[ sched_t ]--------------------------------------------------
/** Definition of the state of an event queue. */
typedef struct event_queue_t {
  sched_type_t         type;
  sched_ops_t          ops;

  gds_fifo_tunable_t   * events;
  state_t              * state;
} event_queue_t;

// -----[ sched_t ]--------------------------------------------------
/** Definition of the state of an event queue. */
typedef struct routing_state_t {
  
    // les rib
    // adj-rib-in and -out pour chaque voisin de chaque noeud.
    // pour chaque noeud :  - rib
    //                      - pour chaque voisin :
    //                                      -adj-rib-in
    //                                      -adj-rib-out
  
  state_t              * state;
}  routing_state_t;

// -----[ sched_t ]--------------------------------------------------
/** Definition of the state of an event queue. */
typedef struct couple_node_rib_t {
    net_node_t *    ptr_to_node;
    

    // les rib
    // adj-rib-in and -out pour chaque voisin de chaque noeud.
    // pour chaque noeud :  - rib
    //                      - pour chaque voisin :
    //                                      -adj-rib-in
    //                                      -adj-rib-out

}  couple_node_rib_t;



// -----[ sched_t ]--------------------------------------------------
/** Definition of the state of an event queue. */
typedef struct routing_state_t {

    // pour l'instant, juste les rib des nodes.
    //tableau de ptr vers des couples.
    couple_node_rib_t **    couples_node_rib;

    // les rib
    // adj-rib-in and -out pour chaque voisin de chaque noeud.
    // pour chaque noeud :  - rib
    //                      - pour chaque voisin :
    //                                      -adj-rib-in
    //                                      -adj-rib-out

  state_t              * state;
}  routing_state_t;


// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct transition_t {
   
  _event_t         * event;
  state_t         * from;
  state_t         * to;
} transition_t;

// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct state_t {
  routing_state_t *    routing_state;
  event_queue_t   *    queue_state;
  transition_t    **   output_transitions;
  graph_t         *    graph;
} state_t;

// -----[ sched_t ]--------------------------------------------------
/** Definition of a state. */
typedef struct graph_t {
  net_node_t     **    list_of_nodes;
  state_t        **    list_of_states;
  unsigned int         nb_nodes;
} graph_t;



#ifdef	__cplusplus
extern "C" {
#endif

      state_t * state_create(graph_t * graph);






#ifdef	__cplusplus
}
#endif

#endif	/* __STATE_H__ */

