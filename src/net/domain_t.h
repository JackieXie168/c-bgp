// ==================================================================
// @(#)domain.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 16/09/2004
// @lastdate 29/07/2005
// ==================================================================

#ifndef __NET_DOMAIN_T_H__
#define __NET_DOMAIN_T_H__

#include <libgds/radix-tree.h>
#include <libgds/types.h>

struct Domain {
  char * pcName;
  SRadixTree * rtNodes;
};
typedef struct Domain SNetDomain;

#endif
