// ==================================================================
// @(#)ecomm.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/12/2002
// @lastdate 03/03/2006
// ==================================================================

#ifndef __ECOMM_H__
#define __ECOMM_H__

#include <libgds/log.h>
#include <libgds/types.h>

#include <bgp/ecomm_t.h>
#include <bgp/peer.h>
#include <net/prefix.h>

// ----- ecomm_val_destroy ------------------------------------------
extern void ecomm_val_destroy(SECommunity ** ppComm);
// ----- ecomm_val_copy ---------------------------------------------
extern SECommunity * ecomm_val_copy(SECommunity * pComm);
// ----- ecomm_create -----------------------------------------------
extern SECommunities * ecomm_create();
// ----- ecomm_destroy ----------------------------------------------
extern void ecomm_destroy(SECommunities ** ppComms);
// ----- ecomm_length -----------------------------------------------
extern unsigned char ecomm_length(SECommunities * pComms);
// ----- ecomm_get_index --------------------------------------------
extern SECommunity * ecomm_get_index(SECommunities * pComms,
				     int iIndex);
// ----- ecomm_copy -------------------------------------------------
extern SECommunities * ecomm_copy(SECommunities * pComms);
// ----- ecomm_add --------------------------------------------------
extern int ecomm_add(SECommunities ** ppComms, SECommunity * pComm);
// ----- ecomm_strip_non_transitive ---------------------------------
extern void ecomm_strip_non_transitive(SECommunities ** ppComms);
// ----- ecomm_dump -------------------------------------------------
extern void ecomm_dump(SLogStream * pStream, SECommunities * pComms,
		       int iText);
// ----- ecomm_equals -----------------------------------------------
extern int ecomm_equals(SECommunities * pCommunities1,
			SECommunities * pCommunities2);
// ----- ecomm_red_create -------------------------------------------
extern SECommunity * ecomm_red_create_as(unsigned char uActionType,
					 unsigned char uActionParam,
					 uint16_t uAS);
// ----- ecomm_red_match --------------------------------------------
extern int ecomm_red_match(SECommunity * pComm, SBGPPeer * pPeer);
// ----- ecomm_red_dump ---------------------------------------------
extern void ecomm_red_dump(SLogStream * pStream, SECommunity * pComm);
// ----- ecomm_val_dump ---------------------------------------------
extern void ecomm_val_dump(SLogStream * pStream, SECommunity * pComm,
			   int iText);
#ifdef __EXPERIMENTAL__
// ----- ecomm_depref_create ----------------------------------------
extern SECommunity * ecomm_pref_create(uint32_t uPref);
// ----- ecomm_pref_dump --------------------------------------------
extern void ecomm_pref_dump(SLogStream * pStream, SECommunity * pComm);
// ----- ecomm_pref_get ---------------------------------------------
extern uint32_t ecomm_pref_get(SECommunity * pComm);
#endif /* __EXPERIMENTAL__ */

#endif
