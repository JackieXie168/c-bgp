// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 09/03/2004
// ==================================================================

#include <net/link.h>

// ----- link_set_state ---------------------------------------------
/**
 *
 */
void link_set_state(SNetLink * pLink, uint8_t uFlag, int iState)
{
  if (iState)
    pLink->uFlags|= uFlag;
  else
    pLink->uFlags&= ~uFlag;
}

// ----- link_get_state ---------------------------------------------
/**
 *
 */
int link_get_state(SNetLink * pLink, uint8_t uFlag)
{
  return (pLink->uFlags & uFlag) != 0;
}

// ----- link_set_igp_weight ----------------------------------------
/**
 *
 */
void link_set_igp_weight(SNetLink * pLink, uint32_t uIGPweight)
{
  pLink->uIGPweight= uIGPweight;
}

// ----- link_get_igp_weight ----------------------------------------
/**
 *
 */
uint32_t link_get_igp_weight(SNetLink * pLink)
{
  return pLink->uIGPweight;
}

// ----- link_dump --------------------------------------------------
void link_dump(FILE * pStream, SNetLink * pLink)
{
  ip_address_dump(pStream, pLink->tAddr);
  fprintf(pStream, "/32\t%u\t%u", pLink->tDelay, pLink->uIGPweight);
  if (link_get_state(pLink, NET_LINK_FLAG_UP))
    fprintf(pStream, "\tUP");
  else
    fprintf(pStream, "\tDOWN");
  if (link_get_state(pLink, NET_LINK_FLAG_TUNNEL))
    fprintf(pStream, "\tTUNNEL");
  else
    fprintf(pStream, "\tDIRECT");
  if (link_get_state(pLink, NET_LINK_FLAG_IGP_ADV))
    fprintf(pStream, "\tIGP_ADV");
}
