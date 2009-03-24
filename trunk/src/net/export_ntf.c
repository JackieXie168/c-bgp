// ==================================================================
// @(#)export_ntf.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_ntf.c,v 1.4 2009-03-24 16:16:04 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <net/error.h>
#include <net/network.h>

// -----[ net_export_ntf ]-------------------------------------------
int net_export_ntf(gds_stream_t * stream, network_t * network)
{
  return EUNSUPPORTED;
}

