// ==================================================================
// @(#) origin.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @data 18/12/2009
// $Id: origin.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <bgp/attr/origin.h>

static char * ORIGIN_NAMES[BGP_ORIGIN_MAX]=
  { "IGP", "EGP", "INCOMPLETE" };

// -----[ bgp_origin_from_str ]--------------------------------------
int bgp_origin_from_str(const char * str, bgp_origin_t * origin_ref)
{
  bgp_origin_t origin= 0;
  while (origin < BGP_ORIGIN_MAX) {
    if (!strcmp(str, ORIGIN_NAMES[origin])) {
      *origin_ref= origin;
      return 0;
    }
    origin++;
  }
  return -1;
}

// -----[ bgp_origin_to_str ]----------------------------------------
const char * bgp_origin_to_str(bgp_origin_t origin)
{
  if (origin >= BGP_ORIGIN_MAX)
    return NULL;
  return ORIGIN_NAMES[origin];
}
