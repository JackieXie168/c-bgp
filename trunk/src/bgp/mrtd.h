// ==================================================================
// @(#)mrtd.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// $Id: mrtd.h,v 1.9 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __MRTD_H__
#define __MRTD_H__

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/as_t.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>
#include <bgp/routes_list.h>
#include <bgp/route-input.h>

typedef uint8_t mrtd_input_t;

// ----- MRT file types -----
#define MRTD_TYPE_INVALID  0
#define MRTD_TYPE_RIB      'B' /* Best route */
#define MRTD_TYPE_UPDATE   'A' /* Advertisement */
#define MRTD_TYPE_WITHDRAW 'W' /* Withdraw */

// ----- MRT file formats -----
#define MRT_FORMAT_ASCII  0
#define MRT_FORMAT_BINARY 1

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // ASCII MRT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- mrtd_create_path -----------------------------------------
  SBGPPath * mrtd_create_path(const char * path);
  // ----- mrtd_route_from_line -------------------------------------
  bgp_route_t * mrtd_route_from_line(const char * line,
				     net_addr_t * peer_addr,
				     unsigned int * peer_asn);
  // ----- mrtd_msg_from_line ---------------------------------------
  bgp_msg_t * mrtd_msg_from_line(bgp_router_t * router, bgp_peer_t * peer,
			       const char * line);
  // ----- mrtd_ascii_load_routes -----------------------------------
  int mrtd_ascii_load(const char * file_name, FBGPRouteHandler handler,
		      void * ctx);


  ///////////////////////////////////////////////////////////////////
  // BINARY MRT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

#ifdef HAVE_BGPDUMP
  // -----[ mrtd_binary_load ]---------------------------------------
  int mrtd_binary_load(const char * file_name, FBGPRouteHandler handler,
		       void * ctx);
#endif


  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION & FINALIZATION
  ///////////////////////////////////////////////////////////////////

  // ----- _mrtd_destroy --------------------------------------------
  void _mrtd_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __MRTD_H__ */
