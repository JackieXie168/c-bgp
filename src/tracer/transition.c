

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>

#include <libgds/memory.h>
#include <libgds/stack.h>



#include <tracer/transition.h>

#include "transition.h"

transition_t * transition_create(  struct _event_t * event, unsigned int num_trans)
{
    transition_t * transi = (transition_t *) MALLOC ( sizeof(transition_t) );
    transi->event=event;
    transi->from=NULL;
    transi->to=NULL;
    transi->num_trans=num_trans;
    return transi;
}
transition_t * transition_create_from(  struct _event_t * event , struct state_t * from, unsigned int num_trans)
{
    transition_t * transi = (transition_t *) MALLOC ( sizeof(transition_t) );
    transi->event=event;
    transi->from=from;
    transi->to=NULL;
    transi->num_trans=num_trans;
    return transi;
}