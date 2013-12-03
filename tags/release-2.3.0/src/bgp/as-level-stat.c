// ==================================================================
// @(#)as-level-stat.c
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/06/2007
// @lastdate 21/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <bgp/as.h>
#include <bgp/as-level.h>
#include <bgp/as-level-types.h>

#define MAX_DEGREE MAX_AS

// -----[ aslevel_stat_degree ]--------------------------------------
void aslevel_stat_degree(SLogStream * pStream, SASLevelTopo * pTopo,
			 int iCumulated, int iNormalized, int iInverse)
{
  unsigned int auDegreeFrequency[MAX_DEGREE];
  unsigned int uIndex;
  unsigned int uDegree, uMaxDegree= 0;
  double dValue, dValue2;
  SASLevelDomain * pDomain;
  unsigned int uNumValues;

  memset(auDegreeFrequency, 0, sizeof(auDegreeFrequency));
  
  uNumValues= 0;
  for (uIndex= 0; uIndex < aslevel_topo_num_nodes(pTopo); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];
    uDegree= ptr_array_length(pDomain->pNeighbors);
    if (uDegree > uMaxDegree)
      uMaxDegree= uDegree;
    assert(uDegree < MAX_DEGREE);
    auDegreeFrequency[uDegree]++;
    uNumValues++;
  }

  dValue= 0;
  for (uIndex= 0; uIndex < uMaxDegree; uIndex++) {
    dValue2= auDegreeFrequency[uIndex];
    if (iNormalized)
      dValue2= dValue2/(1.0*uNumValues);
    if (iCumulated)
      dValue+= dValue2;
    else
      dValue= dValue2;
    if (iInverse)
      log_printf(pStream, "%d\t%f\n", uIndex, 1-dValue);
    else
      log_printf(pStream, "%d\t%f\n", uIndex, dValue);
  }
}
