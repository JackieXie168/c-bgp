// ==================================================================
// @(#)qos_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/11/2002
// @lastdate 20/11/2003
// ==================================================================

#ifndef __BGP_QOS_T_H__
#define __BGP_QOS_T_H__

#include <libgds/types.h>

#include <net/network.h>

// Minimum delay information
typedef struct {
  net_link_delay_t tDelay;
  net_link_delay_t tMean;
  uint8_t uWeight;
} qos_delay_t;

// Maximum reservable bandwidth
typedef struct {
  uint32_t uBandwidth;
  uint32_t uMean;
  uint8_t uWeight;
} qos_bandwidth_t;

#endif
