// ==================================================================
// @(#)ip6.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/01/2008
// $Id: ip6.h,v 1.3 2009-03-24 16:13:18 bqu Exp $
// ==================================================================

#ifndef __NET_IP6_H__
#define __NET_IP6_H__

// ----- IPv6 address -----
typedef uint8_t ip6_addr_t[8];
typedef uint8_t ip6_mask_t;
#define IP6_ADDR_MAX (net_addr6_t) {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

// ----- IPv6 prefix -----
typedef struct {
  ip6_addr_t network;
  ip6_mask_t mask_len;
} ip6_pfx_t;

// ----- IPv6 destination -----
typedef struct {
  uint8_t type;
  union {
    ip6_pfx_t prefix;
    ip6_addr_t addr;  
    };
} ip6_dest_t;

#endif /* __NET_IP6_H__ */
