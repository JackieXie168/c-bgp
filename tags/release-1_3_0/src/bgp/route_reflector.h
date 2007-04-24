// ==================================================================
// @(#)route_reflector.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/12/2003
// @lastdate 28/09/2006
// ==================================================================

#ifndef __BGP_ROUTE_REFLECTOR_H__
#define __BGP_ROUTE_REFLECTOR_H__

#include <libgds/array.h>
#include <libgds/log.h>
#include <libgds/types.h>

#include <stdio.h>

#include <net/prefix.h>

// A CLUSTER_ID is a 4-bytes value (RFC1966)
typedef uint32_t cluster_id_t;

// A CLUSTER_ID_LIST is the list of clusters traversed by a route (RFC1966)
typedef SUInt32Array SClusterList;
// ---- cluster_list_create -----------------------------------------
#define cluster_list_create() \
          (SClusterList *) uint32_array_create(0)
// ---- cluster_list_append -----------------------------------------
#define cluster_list_append(PL, C) \
          _array_append((SArray *) PL, &C)
// ----- cluster_list_copy ------------------------------------------
#define cluster_list_copy(PL) \
          (SClusterList *) _array_copy((SArray *) PL)
// ----- cluster_list_length ----------------------------------------
#define cluster_list_length(PL) \
          _array_length((SArray *) PL)

// ----- cluster_list_destroy ---------------------------------------
extern void cluster_list_destroy(SClusterList ** ppClusterList);
// ----- cluster_list_dump ------------------------------------------
extern void cluster_list_dump(SLogStream * pStream, SClusterList * pClusterList);
// ----- cluster_list_equals -------------------------------------------
extern int cluster_list_equals(SClusterList * pClusterList1,
			       SClusterList * pClusterList2);
// ----- cluster_list_contains --------------------------------------
extern int cluster_list_contains(SClusterList * pClusterList,
				 cluster_id_t tClusterID);

// -----[ originator_equals ]----------------------------------------
extern int originator_equals(net_addr_t * pOrig1, net_addr_t * pOrig2);

#endif
