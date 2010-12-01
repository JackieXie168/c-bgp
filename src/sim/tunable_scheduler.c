// ==================================================================
// @(#)tunable_scheduler.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/07/2003
// $Id: static_scheduler.c,v 1.16 2009-03-10 13:13:25 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

#include <libgds/fifo_tunable.h>
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <sim/tunable_scheduler.h>
#include <bgp/types.h>

#include <net/network.h>

#define EVENT_QUEUE_DEPTH 256

///////////////// une liste d'id des events qui m'ont généré (comme un as path, mais plutot un event path...)
///////////// premier idée ...

/*typedef struct _list_ids_t {
int            id;
_list_ids_t *   next;
} _list_ids_t;

int _id_in_list(int id, _list_ids_t * maillon, int curr_id)
{
if(maillon == NULL) return -1;
else{
if (maillon->id == id)
return 0;
else return _id_in_list(id, maillon->next);
}
}

_list_ids_t * add_id_to_list(int id, _list_ids_t * maillon)
{
_list_ids_t * new_liste = (_list_ids_t *) MALLOC(sizeof(_list_ids_t));
new_liste->id = id;
new_liste->next = maillon;
return new_liste;
}
int _event_get_id(_event_t * ev)
{
if (ev->ids == NULL)
return -1;
else return ev->ids->id;
}
*/


typedef struct {
const sim_event_ops_t * ops;
void                  * ctx;
//int                     id;
//int                     generator;
} _event_t;








// -----[ _event_create ]--------------------------------------------
static inline _event_t * _event_create_tunable(const sim_event_ops_t * ops,
				       void * ctx)
{
 _event_t * event= (_event_t *) MALLOC(sizeof(_event_t));
  event->ops= ops;
  event->ctx= ctx;
  return event;
}

// -----[ _event_destroy ]-------------------------------------------
static void _event_destroy_tunable(_event_t ** event_ref)
{
  if (*event_ref != NULL) {
    FREE(*event_ref);
    *event_ref= NULL;
  }
}
/*
net_msg_t * message_deep_copy(net_msg_t * msg)
{
    net_msg_t * new_msg = ( net_msg_t * ) MALLOC(sizeof( net_msg_t ));
    new_msg->dst_addr = msg->dst_addr;
    new_msg->src_addr = msg->src_addr;
    new_msg->ttl = msg->ttl;
    new_msg->protocol = msg->protocol;
    new_msg->tos = msg->tos;
    new_msg->ops=msg->ops;
    //copie de payload :
    new_msg->payload = ;
    //copie d'option IP.
    new_msg->opts= ;

}*/

// -----[ _fifo_event_destroy ]--------------------------------------
void *  fifo_tunable_event_deep_copy(void * event)
{
     _event_t * new_event = ( _event_t * ) MALLOC(sizeof( _event_t ));
     new_event->ops=((_event_t *) event)->ops;
     new_event->ctx=(net_send_ctx_t *) MALLOC( sizeof(net_send_ctx_t) );
     net_send_ctx_t * send_ctx= (net_send_ctx_t *) ((_event_t *) event)->ctx;     
     ((net_send_ctx_t *)  new_event->ctx)->dst_iface = send_ctx->dst_iface ;
     ((net_send_ctx_t *)  new_event->ctx)->msg = message_copy(send_ctx->msg);
     return new_event;
}




// -----[ _fifo_event_destroy ]--------------------------------------
static void _fifo_tunable_event_destroy(void ** item_ref)
{
  _event_t ** event_ref= (_event_t **) item_ref;
  _event_t * event= *event_ref;

  if (event == NULL)
    return;
  
  if ((event->ops != NULL) && (event->ops->destroy != NULL))
    event->ops->destroy(event->ctx);
  _event_destroy_tunable(event_ref);
}

// -----[ _log_progress ]--------------------------------------------
static inline void _log_progress_tunable(sched_tunable_t * sched)
{
  struct timeval tv;

  if (sched->pProgressLogStream == NULL)
    return;

  assert(gettimeofday(&tv, NULL) >= 0);
  stream_printf(sched->pProgressLogStream, "%d\t%.0f\t%d\n",
		sched->cur_time,
		((double) tv.tv_sec)*1000000 + (double) tv.tv_usec,
		fifo_tunable_depth(sched->events));
}

// -----[ _destroy ]-------------------------------------------------
static void _destroy_tunable(sched_t ** sched_ref)
{
  sched_tunable_t * sched;
  if (*sched_ref != NULL) {

    sched= (sched_tunable_t *) *sched_ref;

    // Free private part
    if (sched->events->current_depth > 0)
      cbgp_warn("%d event%s still in simulation queue.\n",
		sched->events->current_depth,
		(sched->events->current_depth>1?"s":""));
    fifo_tunable_destroy(&sched->events);
    if (sched->pProgressLogStream != NULL)
      stream_destroy(&sched->pProgressLogStream);

    FREE(sched);
    *sched_ref= NULL;
  }
}

// -----[ _cancel ]--------------------------------------------------
static void _cancel_tunable(sched_t * self)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  sched->cancelled= 1;
}

// -----[ _clear ]---------------------------------------------------
static void _clear_tunable(sched_t * self)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  fifo_tunable_destroy(&sched->events);
  sched->events= fifo_tunable_create(EVENT_QUEUE_DEPTH, _fifo_tunable_event_destroy);
  fifo_tunable_set_option(sched->events, FIFO_OPTION_GROW_EXPONENTIAL, 1);
}

// -----[ _run ]-----------------------------------------------------
/**
 * Process the events in the global linear queue.
 *
 * Arguments:
 * - iNumSteps: number of events to process during this run (if -1,
 *   process events until queue is empty).
 */
static net_error_t _run_tunable(sched_t * self, unsigned int num_steps)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  _event_t * event;

  sched->cancelled= 0;

  while ((!sched->cancelled) &&
	 ((event= (_event_t *) fifo_tunable_pop(sched->events)) != NULL)) {

    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "=====<<< EVENT %2.2f >>>=====\n",
	      (double) sched->cur_time);

    /////////////////Stefan
    //printf("event : \n");
    //printf("  ctx : %s\n" , event->ctx);
    //printf("  ctx : %s\n" , event->ctx);

    /////////////////Stefan

    event->ops->callback(sched->sim, event->ctx);
    _event_destroy_tunable(&event);
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\n");

    // Update simulation time
    sched->cur_time++;

    // Limit on simulation time ??
    if ((sched->sim->max_time > 0) &&
	(sched->cur_time >= sched->sim->max_time))
      return ESIM_TIME_LIMIT;

    // Limit on number of steps
    if (num_steps > 0) {
      num_steps--;
      if (num_steps == 0)
	break;
    }

    _log_progress_tunable(sched);
  }
  return ESUCCESS;
}




// -----[ _num_events ]----------------------------------------------
/**
 * Return the number of queued events.
 */
static unsigned int _num_events_tunable(sched_t * self)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  return sched->events->current_depth;
}

// -----[ _event_at ]------------------------------------------------
void * _event_at_tunable(sched_t * self, unsigned int index)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;

  depth= sched->events->current_depth;
  if (index >= depth)
    return NULL;

  max_depth= sched->events->max_depth;
  start= sched->events->start_index;
  return ((_event_t *) sched->events->items[(start+index) % max_depth])->ctx;

}

// -----[ _post ]----------------------------------------------------
static int _post_tunable(sched_t * self, const sim_event_ops_t * ops,
		 void * ctx, double time, sim_time_t time_type)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  _event_t * event= _event_create_tunable(ops, ctx);
  return fifo_tunable_push(sched->events, event);
}

// -----[ _dump_events ]---------------------------------------------
/**
 * Return information 
 */
static void _dump_events_tunable(gds_stream_t * stream, sched_t * self)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= sched->events->current_depth;
  max_depth= sched->events->max_depth;
  start= sched->events->start_index;
  stream_printf(stream, "Tunable Scheduler :\nNumber of events queued: %u (%u)\n",
	     depth, max_depth);
  for (index= 0; index < depth; index++) {
    event= (_event_t *) sched->events->items[(start+index) % max_depth];
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

// -----[ _setFirst ]---------------------------------------------
/**
 * Return information
 */
static int _setFirst_tunable(sched_t * self,  unsigned int nb)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  fifo_tunable_set_first(sched->events,  nb);
    return ESUCCESS;
}

// -----[ _dump_events ]---------------------------------------------
/**
 * Return information
 */
static int _swap_tunable(sched_t * self,  unsigned int nb1, unsigned int nb2)
{
    printf("**********\nATTENTION\n,  a revoir !  _swap_tunable  : vérifier le comportement.");
  sched_tunable_t * sched= (sched_tunable_t *) self;
  fifo_tunable_set_first(sched->events,  nb1);
  return ESUCCESS;
  /* _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= sched->events->current_depth;
  max_depth= sched->events->max_depth;
  start= sched->events->start_index;
  stream_printf(stream, "Tunable Scheduler :\nNumber of events queued: %u (%u)\n",
	     depth, max_depth);
  for (index= 0; index < depth; index++) {
    event= (_event_t *) sched->events->items[(start+index) % max_depth];
    stream_printf(stream, "%d\t(%d) ", index, (start+index) % max_depth);
    stream_flush(stream);
    if (event->ops->dump != NULL) {
      event->ops->dump(stream, event->ctx);
    } else {
      stream_printf(stream, "unknown");
    }
    stream_printf(stream, "\n");
  }*/
}

static int _bringForward_tunable(sched_t * self,  unsigned int num)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  fifo_tunable_bringForward(sched->events,  num);
  return ESUCCESS;
}

// -----[ _set_log_progress ]----------------------------------------
/**
 *
 */
static void _set_log_progress_tunable(sched_t * self, const char * filename)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;

  if (sched->pProgressLogStream != NULL)
    stream_destroy(&sched->pProgressLogStream);

  if (filename != NULL) {
    sched->pProgressLogStream= stream_create_file(filename);
    stream_printf(sched->pProgressLogStream, "# C-BGP Queue Progress\n");
    stream_printf(sched->pProgressLogStream, "# <step> <time (us)> <depth>\n");
  }
}

// -----[ _cur_time ]------------------------------------------------
static double _cur_time_tunable(sched_t * self)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;
  return (double) sched->cur_time;
}


int isOpenSessionMsg(_event_t * event)
{
   //printf("\t @isOpenSessionMsg\n");
   net_send_ctx_t * send_ctx= (net_send_ctx_t *) event->ctx;
   net_msg_t * msg = send_ctx->msg;
   //message_dump( gdsout , msg);
   bgp_msg_type_t type = ( (bgp_msg_t * ) msg ->payload)->type;
   if (type == BGP_MSG_TYPE_OPEN) return 1;
   return 0;
}

int get_index_of_next_Open_Event(sched_tunable_t * self)
{
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  
  depth= self->events->current_depth;
  max_depth= self->events->max_depth;
  start= self->events->start_index;

    unsigned int i ;
      //printf("@get index of next open event !\n");
      //printf(" nb event total : %d\n",  _num_events_tunable(self));
    for(i= 0 ; i < _num_events_tunable(self) ; i++)
    {
        //printf("event %d :  ",i);
        //event = (_event_t *) _event_at_tunable(self,i);
        event= (_event_t *) self->events->items[(start+i) % max_depth];
        if(isOpenSessionMsg(event))
            return i;
    }

    return -1;
}



static net_error_t _runOpenSessions_tunable(sched_t * self)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;  
  int index = -2;
  //printf("@runOpenSessions !\n");
  while((index = get_index_of_next_Open_Event(sched))!=-1)
  {
      //printf("Premier event : num : %d",index);
      //printf("open event :  event en position %d",index);
      _bringForward_tunable(self, index);
      _run_tunable(self, 1);
  }


  return ESUCCESS;
}


void * get_event_at(sched_t * self, unsigned int i)
{
  sched_tunable_t * sched= (sched_tunable_t *) self;

  if(sched->events->current_depth > i )
  {
        return sched->events->items[(sched->events->start_index + i) % sched->events->max_depth];
  }
  else
  {
      return NULL;
  }
}


// -----[ static_scheduler_create ]---------------------------------
/**
 *
 */
sched_t * sched_tunable_create(simulator_t * sim)
{
  sched_tunable_t * sched=
    (sched_tunable_t *) MALLOC(sizeof(sched_tunable_t));

  // Initialize public part (type + ops)
  sched->type= SCHEDULER_TUNABLE;
  sched->sim= sim;
  sched->ops.destroy        = _destroy_tunable;
  sched->ops.cancel         = _cancel_tunable;
  sched->ops.clear          = _clear_tunable;
  sched->ops.run            = _run_tunable;
  sched->ops.post           = _post_tunable;
  sched->ops.num_events     = _num_events_tunable;
  sched->ops.event_at       = _event_at_tunable;
  sched->ops.dump_events    = _dump_events_tunable;
  sched->ops.set_log_process= _set_log_progress_tunable;
  sched->ops.cur_time       = _cur_time_tunable;
  sched->ops.set_first      = _setFirst_tunable;
  sched->ops.swap           = _swap_tunable;
  sched->ops.bringForward   = _bringForward_tunable;
  sched->ops.runOpenSessions= _runOpenSessions_tunable;

  // Initialize private part
  sched->events= fifo_tunable_create(EVENT_QUEUE_DEPTH, _fifo_tunable_event_destroy);
  fifo_tunable_set_option(sched->events, FIFO_OPTION_GROW_EXPONENTIAL, 1);
  sched->cur_time= 0;
  sched->pProgressLogStream= NULL;

  return (sched_t *) sched;
}
