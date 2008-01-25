// ==================================================================
// @(#)ip.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/12/2002
// @lastdate 14/01/2008
// ==================================================================

#ifndef __NET_IP_H__
#define __NET_IP_H__

#define IPV4_TO_INT(A,B,C,D) (((((uint32_t)(A))*256 +(uint32_t)(B))*256 +(uint32_t)( C))*256 +(uint32_t)(D))
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
#define NET_DEST_INVALID 0
#define NET_DEST_ADDRESS 1
#define NET_DEST_PREFIX  2
#define NET_DEST_ANY     3
typedef union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
} UNetDest;

typedef struct {
  uint8_t tType;
  /*union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
    };*/
  UNetDest uDest;
} SNetDest;

#endif /** __NET_IP_H__ */
