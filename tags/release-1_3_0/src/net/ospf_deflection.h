// ==================================================================
// @(#)ospf_deflection.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 8/09/2005
// @lastdate 8/09/2005
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
