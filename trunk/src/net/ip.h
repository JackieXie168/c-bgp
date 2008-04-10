// ==================================================================
// @(#)ip.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/12/2002
// $Id: ip.h,v 1.4 2008-04-10 11:27:00 bqu Exp $
// ==================================================================

#ifndef __NET_IP_H__
#define __NET_IP_H__

#include <stdlib.h>

#define IPV4(A,B,C,D) (((((uint32_t)(A))*256 +(uint32_t)(B))*256 +(uint32_t)( C))*256 +(uint32_t)(D))
#define IP_ADDR_ANY 0
#define NET_ADDR_ANY IP_ADDR_ANY

// ----- IPv4 address -----
typedef uint32_t net_addr_t;
typedef uint8_t  net_mask_t;
#define MAX_ADDR UINT32_MAX

// ----- IPv4 prefix -----
typedef struct {
  net_addr_t tNetwork;
  net_mask_t uMaskLen;
} ip_pfx_t;
typedef ip_pfx_t SPrefix;

// ----- IPv4 destination -----
typedef enum {
  NET_DEST_INVALID,
  NET_DEST_ADDRESS,
  NET_DEST_PREFIX,
  NET_DEST_ANY,
} net_dest_type_t;

typedef union {
    ip_pfx_t   sPrefix;
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
static inline ip_pfx_t net_prefix(net_addr_t network, net_mask_t len)
{
  ip_pfx_t pfx= {
    .tNetwork= network,
    .uMaskLen= len
  };
  return pfx;
}

// -----[ net_dest_addr ]--------------------------------------------
static inline SNetDest net_dest_addr(net_addr_t addr)
{
  SNetDest sDest= {
    .tType= NET_DEST_ADDRESS,
    .uDest.tAddr= addr
  };
  return sDest;
}

// -----[ net_dest_prefix ]------------------------------------------
static inline SNetDest net_dest_prefix(net_addr_t addr, uint8_t len)
{
  SNetDest sDest= {
    .tType= NET_DEST_PREFIX,
    .uDest.sPrefix.tNetwork= addr,
    .uDest.sPrefix.uMaskLen= len,
  };
  return sDest;
}

// -----[ net_dest_to_prefix ]---------------------------------------
static inline ip_pfx_t net_dest_to_prefix(SNetDest sDest)
{
  ip_pfx_t pfx;
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    pfx.tNetwork= sDest.uDest.tAddr;
    pfx.uMaskLen= 32;
    break;
  case NET_DEST_PREFIX:
    pfx= sDest.uDest.sPrefix;
    break;
  default:
    abort();
  }
  return pfx;
}

#endif /** __NET_IP_H__ */
