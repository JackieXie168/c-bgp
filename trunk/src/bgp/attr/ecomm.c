// ==================================================================
// @(#)ecomm.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// $Id: ecomm.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <libgds/stream.h>
#include <libgds/memory.h>

#include <bgp/attr/ecomm.h>
#include <bgp/types.h>

/** Return the memory size in bytes of an extended community
 *  with NUM values. */
#define ecomms_mem_size(NUM)			\
  (sizeof(bgp_ecomms_t)+			\
   (NUM)*sizeof(bgp_ecomm_t))

/** Copy a value COMM into an extended communities (COMMS), at index INDEX. */
#define ecomms_copy_to(COMMS, INDEX, COMM) \
  memcpy(&((COMMS)->values[(INDEX)]), COMM, sizeof(bgp_ecomm_t))

/** Copy a value COMM from an extended communities (COMMS), at index INDEX. */
#define ecomms_copy_from(COMMS, INDEX, COMM) \
  memcpy(COMM, &((COMMS)->values[(INDEX)]), sizeof(bgp_ecomm_t))


// ----- ecomm_val_create -------------------------------------------
bgp_ecomm_t * ecomm_val_create(unsigned char iana_authority,
			       unsigned char transitive)
{
  bgp_ecomm_t * comm= (bgp_ecomm_t *) MALLOC(sizeof(bgp_ecomm_t));
  comm->iana_authority= (iana_authority?0x01:0x00);
  comm->transitive= (transitive?0x01:0x00);
  return comm;
}

// ----- ecomm_val_destroy ------------------------------------------
void ecomm_val_destroy(bgp_ecomm_t ** comm_ref)
{
  bgp_ecomm_t * comm= *comm_ref;
  if (comm == NULL)
    return;
  FREE(comm);
  *comm_ref= NULL;
}

// ----- ecomm_val_copy ---------------------------------------------
bgp_ecomm_t * ecomm_val_copy(bgp_ecomm_t * comm)
{
  bgp_ecomm_t * new_comm= (bgp_ecomm_t *) MALLOC(sizeof(bgp_ecomm_t));
  memcpy(new_comm, comm, sizeof(bgp_ecomm_t));
  return new_comm;
}

// -----[ ecomms_create ]--------------------------------------------
bgp_ecomms_t * ecomms_create()
{
  bgp_ecomms_t * comms= (bgp_ecomms_t *) MALLOC(ecomms_mem_size(0));
  comms->num= 0;
  return comms;
}

// -----[ ecomms_destroy ]-------------------------------------------
void ecomms_destroy(bgp_ecomms_t ** comms_ref)
{
  bgp_ecomms_t * comms= *comms_ref;
  if (comms == NULL)
    return;
  FREE(comms);
  *comms_ref= NULL;
}

// -----[ ecomms_length ]--------------------------------------------
unsigned char ecomms_length(bgp_ecomms_t * comms)
{
  return comms->num;
}

// -----[ ecomms_get_at ]--------------------------------------------
bgp_ecomm_t * ecomms_get_at(bgp_ecomms_t * comms, unsigned int index)
{
  if (index >= comms->num)
    return NULL;
  return &comms->values[index];
}

// -----[ ecomms_dup ]-----------------------------------------------
bgp_ecomms_t * ecomms_dup(bgp_ecomms_t * comms)
{
  bgp_ecomms_t * new_comms=
    (bgp_ecomms_t *) MALLOC(ecomms_mem_size(comms->num));
  memcpy(new_comms, comms, ecomms_mem_size(comms->num));
  return new_comms;
}

// -----[ ecomms_add ]-----------------------------------------------
int ecomms_add(bgp_ecomms_t ** comms_ref, bgp_ecomm_t * comm)
{
  bgp_ecomms_t * comms=
    (bgp_ecomms_t *) REALLOC(*comms_ref, ecomms_mem_size((*comms_ref)->num+1));
  if (comms == NULL)
    return -1;
  comms->num++;
  ecomms_copy_to(comms, comms->num-1, comm);
  ecomm_val_destroy(&comm);
  *comms_ref= comms;
  return 0;
}

// -----[ ecomms_strip_non_transitive ]------------------------------
void ecomms_strip_non_transitive(bgp_ecomms_t ** comms_ref)
{
  unsigned int index;
  unsigned int last_index= 0;

  // Communities is empty
  if (*comms_ref == NULL)
    return;

  // Find non-transitive communities
  for (index= 0; index < (*comms_ref)->num; index++) {
    if ((*comms_ref)->values[index].transitive) {
      if (index != last_index)
	memcpy(&(*comms_ref)->values[last_index],
	       &(*comms_ref)->values[index], sizeof(bgp_ecomm_t));
      last_index++;
    }
  }

  // No non-transitive communities were found
  if ((*comms_ref)->num == last_index)
    return;

  (*comms_ref)->num= last_index;
  if ((*comms_ref)->num == 0) {
    FREE(*comms_ref);
    *comms_ref= NULL;
  } else {
    *comms_ref=
      (bgp_ecomms_t *) REALLOC(*comms_ref,
			       ecomms_mem_size((*comms_ref)->num));
  }
}

// -----[ ecomm_red_dump ]-------------------------------------------
void ecomm_red_dump(gds_stream_t * stream, bgp_ecomm_t * comm)
{
  asn_t asn1, asn2;
  ip_pfx_t pfx;

  switch ((comm->type_low >> 3) & 0x07) {
  case ECOMM_RED_ACTION_PREPEND:
    stream_printf(stream, "prepend %u", (comm->type_low & 0x07));
    break;
  case ECOMM_RED_ACTION_NO_EXPORT:
    stream_printf(stream, "no-export");
    break;
  case ECOMM_RED_ACTION_IGNORE:
    stream_printf(stream, "ignore");
    break;
  default:
    stream_printf(stream, "???");
  }
  stream_printf(stream, " ");
  switch (comm->value[0]) {
  case ECOMM_RED_FILTER_AS:
    asn1= (comm->value[4] << 8) + comm->value[5];
    stream_printf(stream, "%u", asn1);
    break;
  case ECOMM_RED_FILTER_2AS:
    asn1= (comm->value[2] << 8) + comm->value[3];
    asn2= (comm->value[4] << 8) + comm->value[5];
    stream_printf(stream, "%u/%u", asn1, asn2);
    break;
  case ECOMM_RED_FILTER_CIDR:
    pfx.network= (comm->value[1] << 24) + (comm->value[2] << 16) +
      (comm->value[3] << 8) + comm->value[4];
    pfx.mask= comm->value[5];
    ip_prefix_dump(stream, pfx);
    break;
  case ECOMM_RED_FILTER_AS4:
    asn1= (comm->value[2] << 24) + (comm->value[3] << 16) +
      (comm->value[4] << 8) + comm->value[5];
    stream_printf(stream, "%u", asn1);
    break;
  default:
    stream_printf(stream, "???");
  }
}

// ----- ecomm_val_dump ---------------------------------------------
void ecomm_val_dump(gds_stream_t * stream, bgp_ecomm_t * comm,
		    int text)
{
  uint32_t value;

  // If the extended community must be interpreted, a text value is
  // written. Otherwise, a numeric value is written.
  if (text == ECOMM_DUMP_TEXT) {
    switch (comm->type_high) {
    case ECOMM_RED: ecomm_red_dump(stream, comm); break;
#ifdef __EXPERIMENTAL__
    case ECOMM_PREF: ecomm_pref_dump(stream, comm); break;
#endif
    default:
      stream_printf(stream, "???");
    }
  } else {
    value= ((((((((((comm->iana_authority << 1) +
		     comm->transitive) << 1) +
		   comm->type_high) << 6) +
		 comm->type_low) << 8) +
	       comm->value[0]) << 8) +
	     comm->value[1]);
    stream_printf(stream, "%u:", value);
    value= (((((((comm->value[2]) << 8) +
		 comm->value[3]) << 8) +
	       comm->value[4]) << 8) +
	     comm->value[5]);
    stream_printf(stream, "%u", value);
  }
}

// -----[ ecomm_dump ]-----------------------------------------------
void ecomm_dump(gds_stream_t * stream, bgp_ecomms_t * comms,
		int text)
{
  unsigned int index;

  for (index= 0; index < comms->num; index++) {
    if (index > 0)
      stream_printf(stream, " ");
    ecomm_val_dump(stream, &comms->values[index], text);
  }
}

// -----[ ecomm_equals ]---------------------------------------------
int ecomm_equals(bgp_ecomms_t * comms1,
		 bgp_ecomms_t * comms2)
{
  if (comms1 == comms2)
    return 1;
  if ((comms1 == NULL) || (comms2 == NULL))
     return 0;
  if (comms1->num != comms2->num)
    return 0;
  return (memcmp(comms1->values, comms2->values,
		 comms1->num*sizeof(bgp_ecomm_t)) == 0);
}

// -----[ ecomm_red_create ]-----------------------------------------
bgp_ecomm_t * ecomm_red_create_as(unsigned char action_type,
				  unsigned char action_param,
				  asn_t asn)
{
  bgp_ecomm_t * comm= ecomm_val_create(0, 0);
  comm->type_high= ECOMM_RED;
  comm->type_low= ((action_type & 0x07) << 3) + (action_param & 0x07);
  comm->value[0]= ECOMM_RED_FILTER_AS;
  memset(&comm->value[1], 0, 3);
  memcpy(&comm->value[4], &asn, sizeof(asn_t));
  return comm;
}

// -----[ ecomm_red_match ]------------------------------------------
/**
 * 1 => Matches
 * 0 => Does not match
 */
int ecomm_red_match(bgp_ecomm_t * comm, bgp_peer_t * peer)
{
  asn_t asn;
  
  switch (comm->value[0]) {
  case ECOMM_RED_FILTER_AS:
    asn= (comm->value[4] << 8) + comm->value[5];
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "ecomm_red_match(AS%u <=> AS%u)\n",
		 asn, peer->asn);
    if (asn == peer->asn)
      return 1;
    break;
  case ECOMM_RED_FILTER_2AS:
    // not implemented
    break;
  case ECOMM_RED_FILTER_CIDR:
    // not implemented
    break;
  case ECOMM_RED_FILTER_AS4:
    // not implemented
    break;
  }
  return 0;
}

#ifdef __EXPERIMENTAL__
// -----[ ecomm_pref_create ]----------------------------------------
bgp_ecomm_t * ecomm_pref_create(uint32_t pref)
{
  bgp_ecomm_t * comm= ecomm_val_create(0, 0);
  comm->type_high= ECOMM_PREF;
  comm->type_low= 0;
  comm->value[0]= 0;
  memcpy(&comm->value[1], &pref, sizeof(pref));
  return comm;  
}

// -----[ ecomm_pref_dump ]------------------------------------------
void ecomm_pref_dump(gds_stream_t * stream, bgp_ecomm_t * comm)
{
  uint32_t pref;

  stream_printf(stream, "pref ");
  memcpy(&pref, &comm->value[1], sizeof(pref));
  stream_printf(stream, "%u", pref);
}

// -----[ ecomm_pref_get ]-------------------------------------------
uint32_t ecomm_pref_get(bgp_ecomm_t * comm)
{
  uint32_t pref;

  memcpy(&pref, &comm->value[1], sizeof(pref));
  return pref;
}
#endif /* __EXPERIMENTAL__ */
