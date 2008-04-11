// ==================================================================
// @(#)record-route.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @date 04/08/2003
// @lastdate 05/03/2008
// ==================================================================

#ifndef __NET_RECORD_ROUTE_H__
#define __NET_RECORD_ROUTE_H__

#include <stdio.h>

#include <libgds/log.h>

#include <net/ip_trace.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/net_path.h>

//#define NET_RECORD_ROUTE_SUCCESS         NET_SUCCESS
//#define NET_RECORD_ROUTE_TOO_LONG        NET_ERROR_TIME_EXCEEDED
//#define NET_RECORD_ROUTE_UNREACH         NET_ERROR_HOST_UNREACH
//#define NET_RECORD_ROUTE_DOWN            NET_ERROR_NET_UNREACH
#define NET_RECORD_ROUTE_TUNNEL_UNREACH -4
#define NET_RECORD_ROUTE_TUNNEL_BROKEN  -5
//#define NET_RECORD_ROUTE_LOOP		 NET_ERROR_FWD_LOOP

// -----[ Record-route options ]-----
/** Each option should set a single bit so that they can be
 * logically combined with 'or' */
#define NET_RECORD_ROUTE_OPTION_DELAY      0x01
#define NET_RECORD_ROUTE_OPTION_WEIGHT     0x02
#define NET_RECORD_ROUTE_OPTION_CAPACITY   0x04
#define NET_RECORD_ROUTE_OPTION_LOAD       0x08
#define NET_RECORD_ROUTE_OPTION_LOOPBACK   0x10
#define NET_RECORD_ROUTE_OPTION_DEFLECTION 0x20
#define NET_RECORD_ROUTE_OPTION_QUICK_LOOP 0x40

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
  ip_trace_t *     pTrace;
  net_link_delay_t tDelay;    /* QoS info: delay */
  igp_weight_t tWeight;   /* QoS info: IGP weight */
  net_link_load_t  tCapacity; /* QoS info: max capacity */
  net_link_load_t  tLoad;     /* Traffic volume to load */
  net_tos_t        tTOS;      /* Type-Of-Service (topology) */
  SNetPath *       pDeflectedPath;
} SNetRecordRouteInfo;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- node_record_route ----------------------------------------
  SNetRecordRouteInfo * node_record_route(net_node_t * pNode,
					  SNetDest sDest,
					  net_tos_t tTOS,
					  const uint8_t uOptions,
					  net_link_load_t tLoad);
  // ----- node_dump_recorded_route ---------------------------------
  void node_dump_recorded_route(SLogStream * pStream, net_node_t * pNode,
				SNetDest sDest, net_tos_t tTOS,
				uint8_t uOptions,
				net_link_load_t tLoad);
  // ----- net_record_route_info_destroy ----------------------------
  void net_record_route_info_destroy(SNetRecordRouteInfo ** ppRRInfo);

#ifdef __cplusplus
}
#endif

#endif /* __NET_RECORD_ROUTE_H__ */
