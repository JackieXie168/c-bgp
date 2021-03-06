// ==================================================================
// @(#)igp.h
//
// Simple model of an IGP routing protocol. The model does not require
// exchange of messages. It directly compute the routes based on the
// whole topology.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: igp.h,v 1.5 2009-03-24 16:12:24 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide functions for handling the computation of routes by a
 * simple IGP model over an IGP domain. This model statically computes
 * shortest-path-trees (SPT) without exchanging any message between
 * nodes.
 */

#ifndef __NET_IGP_H__
#define __NET_IGP_H__

#include <net/igp_domain.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/spt.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- igp_compute_domain ---------------------------------------
  /**
   * Compute the shortest-path tree (SPT) and build the routing table
   * for each router within an IGP domain.
   *
   * \param domain   is the target IGP domain.
   * \param keep_spt is a flag that tells whether or not the computed
   *                 SPTs must be kept in each IGP domain's member or
   *                 if the SPTs must be destroyed.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   */
  int igp_compute_domain(igp_domain_t * domain, int keep_spt);

  // -----[ spt_bfs ]------------------------------------------------
  /**
   * Compute the Shortest Path Tree (SPT) from the given source router
   * towards all the other routers in the same IGP domain. The
   * algorithm uses a breadth-first-search (BFS) approach.
   *
   * \param src_node is the node at the root of the SPT.
   * \param domain   is the target IGP domain.
   * \param spt_ref  is a reference to the SPT to be computed.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   */
  net_error_t spt_bfs(net_node_t * src_node, igp_domain_t * domain,
		      spt_t ** spt_ref);


#ifdef __cplusplus
}
#endif

#endif /* __NET_IGP_H__ */

