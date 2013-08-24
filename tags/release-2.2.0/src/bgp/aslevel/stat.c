// ==================================================================
// @(#)as-level-stat.c
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: stat.c,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/types.h>

#define MAX_DEGREE MAX_AS

// -----[ aslevel_stat_degree ]--------------------------------------
void aslevel_stat_degree(gds_stream_t * stream, as_level_topo_t * topo,
			 int cumulated, int normalized, int inverse)
{
  unsigned int auDegreeFrequency[MAX_DEGREE];
  unsigned int index;
  unsigned int degree, max_degree= 0;
  double value, value2;
  as_level_domain_t * domain;
  unsigned int num_values;

  memset(auDegreeFrequency, 0, sizeof(auDegreeFrequency));
  
  num_values= 0;
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    degree= ptr_array_length(domain->neighbors);
    if (degree > max_degree)
      max_degree= degree;
    assert(degree < MAX_DEGREE);
    auDegreeFrequency[degree]++;
    num_values++;
  }

  value= 0;
  for (index= 0; index < max_degree; index++) {
    value2= auDegreeFrequency[index];
    if (normalized)
      value2= value2/(1.0*num_values);
    if (cumulated)
      value+= value2;
    else
      value= value2;
    if (inverse)
      stream_printf(stream, "%d\t%f\n", index, 1-value);
    else
      stream_printf(stream, "%d\t%f\n", index, value);
  }
}
