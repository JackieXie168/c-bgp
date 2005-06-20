// ==================================================================
// @(#)routing.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 05/04/2005
// ==================================================================

#ifndef __NET_ROUTING_T_H__
#define __NET_ROUTING_T_H__

#ifdef __EXPERIMENTAL__
# include <libgds/patricia-tree.h>
#else
# include <libgds/radix-tree.h>
#endif
#include <libgds/array.h>


#define NET_ROUTE_ANY    0xFF
#define NET_ROUTE_STATIC 0x01
#define NET_ROUTE_IGP    0x02
#define NET_ROUTE_BGP    0x04

typedef uint8_t net_route_type_t;

#ifdef __EXPERIMENTAL__
typedef STrie SNetRT;
#else
typedef SRadixTree SNetRT;
#endif

#endif
