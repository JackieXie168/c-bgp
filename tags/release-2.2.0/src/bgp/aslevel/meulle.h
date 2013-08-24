// ==================================================================
// @(#)meulle.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/2007
// $Id: meulle.h,v 1.2 2009-06-25 14:31:03 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_MEULLE_H__
#define __BGP_ASLEVEL_MEULLE_H__

#include <stdio.h>

#include <libgds/stream.h>

#include <net/prefix.h>
#include <net/network.h>

#include <bgp/aslevel/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ meulle_parser ]-----------------------------------------
  int meulle_parser(FILE * file, as_level_topo_t * topo,
		    int * line_number);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_MEULLE_H__ */

