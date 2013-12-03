// ==================================================================
// @(#)ospf_deflection.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 8/09/2005
// $Id: ospf_deflection.h,v 1.3 2009-03-24 16:22:02 bqu Exp $
// ==================================================================

#ifndef __NET_OSPF_DEFLECTION__H__
#define __NET_OSPF_DEFLECTION__H__

#ifdef OSPF_SUPPORT


//----- ospf_deflection_test() -------------------------------------------------
int ospf_deflection_test(uint16_t uOspfDomain);
// ----- ospf_domain_deflection ------------------------------------------------
void ospf_domain_deflection(SIGPDomain * pDomain); 




#endif
#endif
