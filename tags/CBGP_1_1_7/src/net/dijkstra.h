// ==================================================================
// @(#)dijkstra.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 23/02/2004
// ==================================================================

#ifndef __NET_DIJKSTRA_H__
#define __NET_DIJKSTRA_H__

#include <libgds/radix-tree.h>
#include <net/prefix.h>
#include <net/network.h>

typedef struct {
  net_addr_t tNextHop;
  net_link_delay_t uIGPweight;
} SDijkstraInfo;

// ----- dijkstra ---------------------------------------------------
extern SRadixTree * dijkstra(SNetwork * pNetwork, net_addr_t tSrcAddr,
			     SPrefix sPrefix);

#endif
