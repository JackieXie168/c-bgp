// ==================================================================
// @(#)rib_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 04/12/2002
// @lastdate 05/04/2005
// ==================================================================

#ifndef __RIB_T_H__
#define __RIB_T_H__

#ifdef __EXPERIMENTAL__
# include <libgds/patricia-tree.h>
#else
# include <libgds/radix-tree.h>
#endif

#ifdef __EXPERIMENTAL__
typedef STrie SRIB;
#else
typedef SRadixTree SRIB;
#endif

#endif
