// ==================================================================
// @(#)ip.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/12/2002
// @lastdate 15/02/2008
// ==================================================================

#ifndef __NET_IP_H__
#define __NET_IP_H__

#include <stdlib.h>

#define IPV4(A,B,C,D) (((((uint32_t)(A))*256 +(uint32_t)(B))*256 +(uint32_t)( C))*256 +(uint32_t)(D))
#define NET_ADDR_ANY 0

// ----- IPv4 address -----
typedef uint32_t net_addr_t;
typedef uint8_t  net_mask_t;
#define MAX_ADDR UINT32_MAX

// ----- IPv4 prefix -----
typedef struct {
  net_addr_t tNetwork;
  net_mask_t uMaskLen;
} SPrefix;

// ----- IPv4 destination -----
typedef enum {
  NET_DEST_INVALID,
  NET_DEST_ADDRESS,
  NET_DEST_PREFIX,
  NET_DEST_ANY,
} net_dest_type_t;

typedef union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
} UNetDest;


typedef struct {
  net_dest_type_t tType;
  /*union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
    };*/
  UNetDest uDest;
} SNetDest;

// -----[ net_prefix ]-----------------------------------------------
static inline SPrefix net_prefix(net_addr_t tNetwork, net_mask_t tMaskLen)
{
  SPrefix sPrefix= {
    .tNetwork= tNetwork,
    .uMaskLen= tMaskLen
  };
  return sPrefix;
}

// -----[ net_dest_addr ]--------------------------------------------
static inline SNetDest net_dest_addr(net_addr_t tAddr)
{
  SNetDest sDest= {
    .tType= NET_DEST_ADDRESS,
    .uDest.tAddr= tAddr
  };
  return sDest;
}

// -----[ net_dest_prefix ]------------------------------------------
static inline SNetDest net_dest_prefix(net_addr_t tAddr, uint8_t tMaskLen)
{
  SNetDest sDest= {
    .tType= NET_DEST_PREFIX,
    .uDest.sPrefix.tNetwork= tAddr,
    .uDest.sPrefix.uMaskLen= tMaskLen,
  };
  return sDest;
}

// -----[ net_dest_to_prefix ]---------------------------------------
static inline SPrefix net_dest_to_prefix(SNetDest sDest)
{
  SPrefix sPrefix;
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    sPrefix.tNetwork= sDest.uDest.tAddr;
    sPrefix.uMaskLen= 32;
    break;
  case NET_DEST_PREFIX:
    sPrefix= sDest.uDest.sPrefix;
    break;
  default:
    abort();
  }
  return sPrefix;
}

#endif /** __NET_IP_H__ */