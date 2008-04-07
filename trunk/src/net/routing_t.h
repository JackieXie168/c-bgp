// ==================================================================
// @(#)routing_t.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/02/2004
// @lastdate 12/03/2008
// ==================================================================

#ifndef __NET_ROUTING_T_H__
#define __NET_ROUTING_T_H__

# include <libgds/patricia-tree.h>
#include <libgds/array.h>

#define NET_ROUTE_ANY    0xFF
#define NET_ROUTE_DIRECT 0x01
#define NET_ROUTE_STATIC 0x02
#define NET_ROUTE_IGP    0x04
#define NET_ROUTE_BGP    0x08

typedef uint8_t net_route_type_t;

typedef STrie net_rt_t;
typedef net_rt_t SNetRT;

#endif
