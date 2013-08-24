// ==================================================================
// @(#)import_meulle.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/2007
// $Id: import_meulle.h,v 1.2 2008-04-14 09:12:49 bqu Exp $
// ==================================================================

#ifndef __BGP_IMPORT_MEULLE_H__
#define __BGP_IMPORT_MEULLE_H__

#include <libgds/log.h>

#include <bgp/as-level.h>
#include <net/prefix.h>
#include <net/network.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ meulle_parser ]-----------------------------------------
  int meulle_parser(FILE * pStream, SASLevelTopo * pTopo,
		    unsigned int * puLineNumber);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_IMPORT_MEULLE_H__ */

