#ifndef __NET_DOMAIN_T_H__
#define __NET_DOMAIN_T_H__

#include <libgds/radix-tree.h>
#include <libgds/types.h>

struct Domain {
  uint32_t uAS;
  char * pcName;
  SRadixTree * rtNodes;
};
typedef struct Domain SNetDomain;

#endif
