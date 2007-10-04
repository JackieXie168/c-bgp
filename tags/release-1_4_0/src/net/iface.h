// ==================================================================
// @(#)iface.h
//
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/08/2003
// @lastdate 02/08/2005
// ==================================================================

#ifndef __NET_IFACE_H__
#define __NET_IFACE_H__

#include <libgds/array.h>
#include <net/prefix.h>

typedef struct {
  net_addr_t tAddr;
  SPrefix * tMask;
  SPtrArray * pLinks;
} SNetIface;

#endif /* __NET_IFACE_H__ */
