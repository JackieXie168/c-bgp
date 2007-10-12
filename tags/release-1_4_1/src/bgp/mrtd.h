// ==================================================================
// @(#)mrtd.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 21/05/2007
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
  SBGPPath * mrtd_create_path(const char * pcPath);
  // ----- mrtd_route_from_line -------------------------------------
  SRoute * mrtd_route_from_line(const char * pcLine,
				net_addr_t * ptPeerAddr,
				unsigned int * puPeerAS);
  // ----- mrtd_msg_from_line ---------------------------------------
  SBGPMsg * mrtd_msg_from_line(SBGPRouter * pRouter, SBGPPeer * pPeer,
			       const char * pcLine);
  // ----- mrtd_ascii_load_routes -----------------------------------
  int mrtd_ascii_load(const char * pcFileName, FBGPRouteHandler fHandler,
		      void * pContext);


  ///////////////////////////////////////////////////////////////////
  // BINARY MRT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

#ifdef HAVE_BGPDUMP
  // -----[ mrtd_binary_load ]---------------------------------------
  int mrtd_binary_load(const char * pcFileName, FBGPRouteHandler fHandler,
		       void * pContext);
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
