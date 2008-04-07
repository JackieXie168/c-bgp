// ==================================================================
// @(#)types.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/11/2005
// @lastdate 21/11/2005
// ==================================================================

#ifndef __BGP_TYPES_H__
#define __BGP_TYPES_H__

#include <libgds/types.h>
#include <libgds/array.h>

//#include <bgp/as_t.h>
#include <bgp/comm_t.h>
#include <bgp/ecomm_t.h>
//#include <bgp/path.h>
#include <bgp/qos_t.h>
#include <bgp/route_reflector.h>
#include <net/prefix.h>

//#define __ROUTER_LIST_ENABLE__

typedef SPtrArray bgp_routes_t;

// -----[ BGP attribute reference counter ]--------------------------
typedef uint32_t bgp_attr_refcnt_t;

// -----[ BGP route ORIGIN type ]------------------------------------
typedef enum {
  ROUTE_ORIGIN_IGP= 0,        /* static route (see "add network" command) */
  ROUTE_ORIGIN_EGP= 1,        /* route learned through BGP */
  ROUTE_ORIGIN_INCOMPLETE= 2, /* route learned through redistribution */
} bgp_origin_t;

// -----[ AS-Path segment ]------------------------------------------
typedef struct {
  uint8_t uType;       /* Segment type */
  uint8_t uLength;     /* Number of ASs in the value field */
  uint16_t auValue[0]; /* One or more AS numbers */
} SPathSegment;

// -----[ AS-Paths / AS-Trees ]--------------------------------------
/** 
 * There are two types of AS-Paths supported so far. The first one is
 * the simplest and contains the list of segments composing the
 * path. The second type can be used to reduce the memory consumption
 * and is composed of a segment and a previous AS-Path (called the
 * "tail") of the path.
 */
#ifndef __BGP_PATH_TYPE_TREE__
typedef SPtrArray SBGPPath;
#else
struct TBGPPath {
  bgp_attr_refcnt_t tRefCnt;
  SPathSegment * pSegment;
  struct TBGPPath * pPrevious;
};
typedef struct TBGPPath SBGPPath;
#endif

// -----[ BGP route attributes ]-------------------------------------
typedef struct {
  /* Attributes */
  net_addr_t tNextHop;
  bgp_origin_t tOrigin;
  SBGPPath * pASPathRef;
  SCommunities * pCommunities;
  uint32_t uLocalPref;
  uint32_t uMED;
  SECommunities * pECommunities;

  /* Route-Reflection attributes */
  net_addr_t * pOriginator;
  SClusterList * pClusterList;

  /* QoS attributes (experimental) */
#ifdef BGP_QOS
  qos_delay_t tDelay;
  qos_bandwidth_t tBandwidth;
  SPtrArray * pAggrRoutes;
  struct TRoute * pAggrRoute;
  struct TRoute * pEBGPRoute;
#endif

#ifdef __ROUTER_LIST_ENABLE__
  /* Router-List attribute (experimental) */
  SClusterList * pRouterList;
#endif

} SBGPAttr;

// -----[ SBGPRootCause ]--------------------------------------------
typedef uint8_t bgp_rcn_id_t;
typedef struct {
  bgp_rcn_id_t tID;
  struct TBGPRouter * pRouter;
} SBGPRootCause;

#endif /* __BGP_TYPES_H__ */
