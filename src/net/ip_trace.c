// ==================================================================
// @(#)ip_trace.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2008
// @lastdate 01/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/array.h>

#include <net/ip_trace.h>

// -----[ ip_trace_item_node ]---------------------------------------
SIPTraceItem * ip_trace_item_node(SNetNode * pNode,
				  net_addr_t tInIfaceAddr,
				  net_addr_t tOutIfaceAddr)
{
  SIPTraceItem * pItem= (SIPTraceItem *) MALLOC(sizeof(SIPTraceItem));
  pItem->eType= NODE;
  pItem->uItem.pNode= pNode;
  pItem->tInIfaceAddr= tInIfaceAddr;
  pItem->tOutIfaceAddr= tOutIfaceAddr;
  return pItem;
}

// -----[ ip_trace_item_subnet ]-------------------------------------
SIPTraceItem * ip_trace_item_subnet(SNetSubnet * pSubnet)
{
  SIPTraceItem * pItem= (SIPTraceItem *) MALLOC(sizeof(SIPTraceItem));
  pItem->eType= SUBNET;
  pItem->uItem.pSubnet= pSubnet;
  pItem->tInIfaceAddr= NET_ADDR_ANY;
  pItem->tOutIfaceAddr= NET_ADDR_ANY;
  return pItem;
}

// -----[ ip_trace_create ]------------------------------------------
SIPTrace * ip_trace_create()
{
  SIPTrace * pTrace= (SIPTrace *) MALLOC(sizeof(SIPTrace));
  pTrace->pItems= ptr_array_create(0, NULL, NULL);
  return pTrace;
}

// -----[ ip_trace_destroy ]-----------------------------------------
void ip_trace_destroy(SIPTrace ** ppTrace)
{
  if (*ppTrace != NULL) {
    ptr_array_destroy(&(*ppTrace)->pItems);
    FREE(*ppTrace);
    *ppTrace= NULL;
  }
}

// -----[ ip_trace_add ]---------------------------------------------
int ip_trace_add(SIPTrace * pTrace, SIPTraceItem * pItem)
{
  return ptr_array_add(pTrace->pItems, &pItem);
}

// -----[ ip_trace_length ]------------------------------------------
unsigned int ip_trace_length(SIPTrace * pTrace)
{
  return ptr_array_length(pTrace->pItems);
}

// -----[ ip_trace_item_at ]-----------------------------------------
SIPTraceItem * ip_trace_item_at(SIPTrace * pTrace, unsigned int uIndex)
{
  return pTrace->pItems->data[uIndex];
}

// -----[ ip_trace_search ]------------------------------------------
int ip_trace_search(SIPTrace * pTrace, SNetNode * pNode)
{
  unsigned int uIndex;
  SIPTraceItem * pItem;
  for (uIndex= 0; uIndex < ip_trace_length(pTrace); uIndex++) {
    pItem= ip_trace_item_at(pTrace, uIndex);
    if ((pItem->eType == NODE) && (pItem->uItem.pNode == pNode))
      return 1;
  }
  return 0;
}
