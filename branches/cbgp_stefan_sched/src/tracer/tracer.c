

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/str_util.h>

#include <libgds/memory.h>
#include <libgds/stack.h>

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






// concider the simulator current state as the initial state.
void tracer_init(tracer_t * self)
{
     self->graph = graph_create((sched_tunable_t *) self->simulator->sched);
     self->set = 1;
}

int FOR_TESTING_PURPOSE_tracer_go_one_step(tracer_t * self )
{
    if(self->set == 0)
        return -1;

    //actuellement graphe est une simple chaine
    // injecter l'état actuel dans le système (scheduler ou simulator)
    // faire simuler un pas (le premier message sera celui traité)
    // capturer l'état actuel, le mettre dans un nouvel état et le lier au précédent.
    // noter le nouvel état comme l'état actuel.

    
}

void tracer_dump(gds_stream_t * stream, tracer_t * self)
{
    if(self->set == 0)
    {
        sprintf(stream,"Tracer not yet initialized");
    }
    else
    {
        sprintf(stream,"Tracer Ready : ");
    }
}



tracer_t * tracer_create( simulator_t * simulator)
{
  tracer_t * tracer;
  tracer=(tracer_t *) MALLOC(sizeof(tracer_t));

  tracer->simulator = simulator;
  tracer->graph = NULL;
  tracer->set = 0;
  return tracer;
}
