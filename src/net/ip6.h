// ==================================================================
// @(#)ip6.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/01/2008
// $Id: ip6.h,v 1.2 2008-04-10 11:27:00 bqu Exp $
// ==================================================================

#ifndef __NET_IP6_H__
#define __NET_IP6_H__

// ----- IPv6 address -----
typedef uint8_t net_addr6_t[8];
typedef uint8_t net_mask6_t;
#define MAX_ADDR6 (net_addr6_t) {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

// ----- IPv6 prefix -----
typedef struct {
  net_addr6_t tNetwork;
  net_mask6_t uMaskLen;
} SPrefix6;

// ----- IPv6 destination -----
typedef union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
} UNetDest6;
typedef struct {
  uint8_t tType;
  /*union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
    };*/
  UNetDest6 uDest;
} SNetDest6;

#endif /* __NET_IP6_H__ */
