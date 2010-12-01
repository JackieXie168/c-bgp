

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>

#include <libgds/memory.h>
#include <libgds/stack.h>



#include <tracer/transition.h>

transition_t * transition_create(  struct _event_t * event)
{
    transition_t * transi = (transition_t *) MALLOC ( sizeof(transition_t) );
    transi->event=event;
    transi->from=NULL;
    transi->to=NULL;
    return transi;
}
