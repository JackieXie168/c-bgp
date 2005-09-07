// ==================================================================
// @(#)routing_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 02/08/2005
// ==================================================================

#ifndef __NET_ROUTING_T_H__
#define __NET_ROUTING_T_H__

# include <libgds/patricia-tree.h>
#include <libgds/array.h>

#define NET_ROUTE_ANY    0xFF
#define NET_ROUTE_STATIC 0x01
#define NET_ROUTE_IGP    0x02
#define NET_ROUTE_BGP    0x04

typedef uint8_t net_route_type_t;

typedef STrie SNetRT;

#endif
