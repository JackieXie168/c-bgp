// ==================================================================
// @(#)rib_t.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 04/12/2002
// $Id: rib_t.h,v 1.5 2008-06-11 15:14:52 bqu Exp $
// ==================================================================

#ifndef __RIB_T_H__
#define __RIB_T_H__

#include <libgds/patricia-tree.h>

typedef enum {
  RIB_IN,
  RIB_OUT,
  RIB_MAX
} bgp_rib_dir_t;

typedef STrie SRIB;
typedef STrie bgp_rib_t;

#endif
