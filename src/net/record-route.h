// ==================================================================
// @(#)record-route.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 04/08/2003
// @lastdate 08/08/2005
// ==================================================================

#ifndef __NET_RECORD_ROUTE_H__
#define __NET_RECORD_ROUTE_H__

#include <stdio.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/net_path.h>

#define NET_RECORD_ROUTE_SUCCESS         0
#define NET_RECORD_ROUTE_TOO_LONG       -1
#define NET_RECORD_ROUTE_UNREACH        -2
#define NET_RECORD_ROUTE_DOWN           -3
#define NET_RECORD_ROUTE_TUNNEL_UNREACH -4
#define NET_RECORD_ROUTE_TUNNEL_BROKEN  -5
#define NET_RECORD_ROUTE_LOOP		-6

#define NET_RECORD_ROUTE_OPTION_DELAY      0x01
#define NET_RECORD_ROUTE_OPTION_WEIGHT     0x02
#define NET_RECORD_ROUTE_OPTION_DEFLECTION 0x04

// ----- SNetRecordRouteInfo ----------------------------------------
/**
 * This structure is used to store a record of the path from one node
 * towards another. The structure can record additional fields such as
 * the total delay and weight along the route.
 *
 * Note that the weight only makes sense for pure intradomain paths.
 */
typedef struct {
  int              iResult;
  uint8_t          uOptions;
  SNetPath *       pPath;
  net_link_delay_t tDelay;
  uint32_t         uWeight;
  SNetPath *       pDeflectedPath;
} SNetRecordRouteInfo;

// ----- node_record_route ------------------------------------------
extern SNetRecordRouteInfo * node_record_route(SNetNode * pNode,
					       SNetDest sDest,
					       const uint8_t uOptions);
// ----- node_dump_recorded_route -----------------------------------
extern void node_dump_recorded_route(FILE * pStream, SNetNode * pNode,
				     SNetDest sDest,
				     const uint8_t uOptions);
// ----- net_record_route_info_destroy ------------------------------
extern void net_record_route_info_destroy(SNetRecordRouteInfo ** ppRRInfo);


#endif /* __NET_RECORD_ROUTE_H__ */
