// ==================================================================
// @(#)ecomm.h
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 02/12/2002
// @lastdate 23/05/2003
// ==================================================================

#ifndef __ECOMM_H__
#define __ECOMM_H__

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
extern void ecomm_dump(FILE * pStream, SECommunities * pComms);
// ----- ecomm_equals -----------------------------------------------
extern int ecomm_equals(SECommunities * pCommunities1,
			SECommunities * pCommunities2);
// ----- ecomm_red_create -------------------------------------------
extern SECommunity * ecomm_red_create_as(unsigned char uActionType,
					 unsigned char uActionParam,
					 uint16_t uAS);
// ----- ecomm_red_match --------------------------------------------
extern int ecomm_red_match(SECommunity * pComm, SPeer * pPeer);
// ----- ecomm_red_dump ---------------------------------------------
extern void ecomm_red_dump(FILE * pStream, SECommunity * pComm);

#endif
