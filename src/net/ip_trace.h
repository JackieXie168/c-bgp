// ==================================================================
// @(#)ip_trace.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2008
// @lastdate 01/03/2008
// ==================================================================

#ifndef __NET_IP_TRACE_H__
#define __NET_IP_TRACE_H__

#include <libgds/array.h>

#include <net/net_types.h>

typedef enum {
  NODE, SUBNET
} EIPTraceItemType;

typedef union {
  SNetNode * pNode;
  SNetSubnet * pSubnet;
} UIPTraceItem;

typedef struct {
  EIPTraceItemType eType;     /* item type: node / subnet */
  UIPTraceItem     uItem;     /* item */
  net_addr_t       tInIfaceAddr;
  net_addr_t       tOutIfaceAddr;
  void *           pUserData; /* user-data (e.g. hop QoS data) */
} SIPTraceItem;

typedef struct {
  SPtrArray * pItems;
} SIPTrace;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ip_trace_item_node ]-------------------------------------
  SIPTraceItem * ip_trace_item_node(SNetNode * pNode,
				    net_addr_t tInIfaceAddr,
				    net_addr_t tOutIfaceAddr);
  // -----[ ip_trace_item_subnet ]-----------------------------------
  SIPTraceItem * ip_trace_item_subnet(SNetSubnet * pSubnet);

  // -----[ ip_trace_create ]----------------------------------------
  SIPTrace * ip_trace_create();
  // -----[ ip_trace_destroy ]---------------------------------------
  void ip_trace_destroy(SIPTrace ** ppTrace);
  // -----[ ip_trace_add ]-------------------------------------------
  int ip_trace_add(SIPTrace * pTrace, SIPTraceItem * pItem);
  // -----[ ip_trace_length ]----------------------------------------
  unsigned int ip_trace_length(SIPTrace * pTrace);
  // -----[ ip_trace_item_at ]---------------------------------------
  SIPTraceItem * ip_trace_item_at(SIPTrace * pTrace, unsigned int uIndex);
  // -----[ ip_trace_search ]----------------------------------------
  int ip_trace_search(SIPTrace * pTrace, SNetNode * pNode);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IP_TRACE_H__ */
