// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
//         Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// @lastdate 01/07/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <net/net_types.h>
#include <net/subnet.h>
#include <net/link.h>
#include <libgds/memory.h>


/////////////////////////////////////////////////////////////////////
// LINK METHODS
/////////////////////////////////////////////////////////////////////
// ----- create_link_toRouter ----------------------------------------
SNetLink * create_link_toRouter(SNetNode * pNode)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->uDestinationType = NET_LINK_TYPE_ROUTER;
  (pLink->UDestId).tAddr = pNode->tAddr;   
  return pLink;
}

// ----- create_link_toSubnet ----------------------------------------
SNetLink * create_link_toSubnet(SNetSubnet * pSubnet)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  if (subnet_is_transit(pSubnet))
    pLink->uDestinationType = NET_LINK_TYPE_TRANSIT;
  else 
    pLink->uDestinationType = NET_LINK_TYPE_STUB;
  (pLink->UDestId).pSubnet = pSubnet;
  return pLink;
}

//  ----- create_link_toRouter_byAddr ----------------------------------------
SNetLink * create_link_toRouter_byAddr(net_addr_t tAddr)
{
  SNetLink * pLink= (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->uDestinationType = NET_LINK_TYPE_ROUTER;
  (pLink->UDestId).tAddr = tAddr;
  return pLink;
}

// ----- link_get_address ----------------------------------------
net_addr_t link_get_address(SNetLink * pLink)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER )
    return (pLink->UDestId).tAddr;
  else 
  {
    SNetSubnet * pSubnet = (pLink->UDestId).pSubnet;
    return (pSubnet->sPrefix).tNetwork;// tAddr;
  }
}

// ----- link_destroy -----------------------------------------------
/*
 *
 */
void link_destroy(SNetLink ** ppLink)
{
  if (*ppLink != NULL) {
    FREE(*ppLink);
    *ppLink= NULL;
  }
}

// ----- link_get_prefix ----------------------------------------
void link_get_prefix(SNetLink * pLink, SPrefix * pPrefix)
{
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER ){
    pPrefix->tNetwork = (pLink->UDestId).tAddr;
    pPrefix->uMaskLen = 32;
  }
  else 
  {
    pPrefix->tNetwork = ((pLink->UDestId).pSubnet->sPrefix).tNetwork;
    pPrefix->uMaskLen = ((pLink->UDestId).pSubnet->sPrefix).uMaskLen;// tAddr;
  }
}
// ----- link_set_type ---------------------------------------------
void link_set_type(SNetLink * pLink, uint8_t uDestinationType)
{
  pLink->uDestinationType = uDestinationType;
}

// ----- link_set_state ----------------------------------------
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

// ----- link_get_subnet --------------------------------------------------
SNetSubnet * link_get_subnet(SNetLink * pLink){
  if (pLink->uDestinationType != NET_LINK_TYPE_ROUTER)
    return (pLink->UDestId).pSubnet;
  return NULL;
}

// ----- link_dump --------------------------------------------------
void link_dump(FILE * pStream, SNetLink * pLink)
{
  ip_address_dump(pStream, link_get_address(pLink));
  if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER)
    fprintf(pStream, "\tTO ROUTER");
  else if (pLink->uDestinationType == NET_LINK_TYPE_TRANSIT)
    fprintf(pStream, "\tTO TRANSIT");
  else 
    fprintf(pStream, "\tTO SUBNET");
    
  fprintf(pStream, "\t%u\t%u", pLink->tDelay, pLink->uIGPweight);
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

