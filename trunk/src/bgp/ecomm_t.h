// ==================================================================
// @(#)ecomm_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 06/12/2002
// @lastdate 03/01/2005
// ==================================================================

#ifndef __ECOMM_T_H__
#define __ECOMM_T_H__

#include <libgds/types.h>

#include <net/prefix.h>

#define ECOMM_RED 3

#define ECOMM_RED_ACTION_PREPEND 0
#define ECOMM_RED_ACTION_NO_EXPORT 1
#define ECOMM_RED_ACTION_IGNORE 2

#define ECOMM_RED_FILTER_AS 0
#define ECOMM_RED_FILTER_2AS 1
#define ECOMM_RED_FILTER_CIDR 2
#define ECOMM_RED_FILTER_AS4 3

#define ECOMM_DUMP_RAW 0
#define ECOMM_DUMP_TEXT 1

typedef struct {
  unsigned char uIANAAuthority:1;
  unsigned char uTransitive:1;
  unsigned char uTypeHigh:6;
  unsigned char uTypeLow;
  unsigned char auValue[6];
} SECommunity;

typedef struct {
  unsigned char uNum;
  SECommunity asComms[0];
} SECommunities;

#endif
