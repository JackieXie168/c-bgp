// ==================================================================
// @(#)link.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 24/02/2004
// ==================================================================

#ifndef __NET_LINK_H__
#define __NET_LINK_H__

#include <stdlib.h>

#include <net/prefix.h>
#include <libgds/types.h>

#define NET_LINK_DELAY_INT
#define NET_LINK_FLAG_UP     0x01
#define NET_LINK_FLAG_NOTIFY 0x01

#ifdef NET_LINK_DELAY_INT
typedef uint32_t net_link_delay_t;
#define NET_LINK_DELAY_INFINITE MAX_UINT32_T
#else
typedef double net_link_delay_t;
#endif

typedef struct {
  net_addr_t tAddr;
  net_link_delay_t tDelay;
  uint8_t uFlags;
  uint32_t uIGPweight; // Store IGP weight here
} SNetLink;

// ----- link_set_state ---------------------------------------------
extern void link_set_state(SNetLink * pLink, uint8_t uFlag, int iState);
// ----- link_get_state ---------------------------------------------
extern int link_get_state(SNetLink * pLink, uint8_t uFlag);
// ----- link_set_igp_weight ----------------------------------------
extern void link_set_igp_weight(SNetLink * pLink, uint32_t uIGPweight);
// ----- link_get_igp_weight ----------------------------------------
extern uint32_t link_get_igp_weight(SNetLink * pLink);
// ----- link_dump --------------------------------------------------
extern void link_dump(FILE * pStream, SNetLink * pLink);

#endif
