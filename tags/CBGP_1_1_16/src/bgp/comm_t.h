// ==================================================================
// @(#)comm_t.h
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 23/05/2003
// @lastdate 23/05/2003
// ==================================================================

#ifndef __COMM_T_H__
#define __COMM_T_H__

#include <libgds/sequence.h>
#include <libgds/types.h>

#define COMM_NO_EXPORT 0xffffff01
#define COMM_NO_ADVERTISE 0xffffff02
#define COMM_NO_EXPORT_SUBCONFED 0xffffff03

typedef SSequence SCommunities;
typedef uint32_t comm_t;

#endif
