// ==================================================================
// @(#)mrtd.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 24/05/2004
// ==================================================================

#ifndef __MRTD_H__
#define __MRTD_H__

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/as_t.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>

typedef uint8_t mrtd_input_t;

#define MRTD_TYPE_RIB      'B' /* Best route */
#define MRTD_TYPE_UPDATE   'A' /* Advertisement */
#define MRTD_TYPE_WITHDRAW 'W' /* Withdraw */

// ----- mrtd_route_from_line ---------------------------------------
extern SRoute * mrtd_route_from_line(SBGPRouter * pRouter,
				     char * pcLine);
// ----- mrtd_msg_from_line -----------------------------------------
extern SBGPMsg * mrtd_msg_from_line(SBGPRouter * pRouter,
				    SPeer * pPeer,
				    char * pcLine);
// ----- mrtd_load_routes -------------------------------------------
extern SPtrArray * mrtd_load_routes(SBGPRouter * pRouter,
				    char * pcFileName);

#endif
