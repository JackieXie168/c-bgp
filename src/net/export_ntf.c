// ==================================================================
// @(#)export_ntf.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_ntf.c,v 1.3 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <net/error.h>
#include <net/network.h>

// -----[ net_export_ntf ]-------------------------------------------
int net_export_ntf(SLogStream * pStream, network_t * network)
{
  return EUNSUPPORTED;
}

