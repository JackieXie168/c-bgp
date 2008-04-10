// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/02/2004
// $Id: routing.c,v 1.27 2008-04-10 11:32:25 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/network.h>
#include <net/routing.h>
#include <ui/output.h>

#include <assert.h>
#include <string.h>

// ----- route_nexthop_compare --------------------------------------
/**
 *
 */
int route_nexthop_compare(rt_entry_t sNH1, rt_entry_t sNH2)
{
  if (sNH1.gateway > sNH2.gateway)
    return 1;
  if (sNH1.gateway < sNH2.gateway)
    return -1;
  
  if (sNH1.oif->tIfaceAddr >
      sNH2.oif->tIfaceAddr)
    return 1;
  if (sNH1.oif->tIfaceAddr <
      sNH2.oif->tIfaceAddr)
    return -1;

  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// ROUTE (rt_info_t)
//
/////////////////////////////////////////////////////////////////////

// -----[ net_route_info_create ]------------------------------------
/**
 *
 */
rt_info_t * net_route_info_create(ip_pfx_t prefix,
				  net_iface_t * oif,
				  net_addr_t gateway,
				  uint32_t metric,
				  net_route_type_t type)
{
  rt_info_t * rtinfo= (rt_info_t *) MALLOC(sizeof(rt_info_t));
  rtinfo->prefix= prefix;
  rtinfo->next_hop.gateway= gateway;
  rtinfo->next_hop.oif= oif;
  //rtinfo->pNextHops= NULL;
  rtinfo->metric= metric;
  rtinfo->type= type;
  return rtinfo;
}

// -----[ net_route_info_destroy ]-----------------------------------
/**
 *
 */
void net_route_info_destroy(rt_info_t ** rtinfo_ref)
{
  if (*rtinfo_ref != NULL) {
    FREE(*rtinfo_ref);
    *rtinfo_ref= NULL;
  }
}

// ----- _rt_info_list_cmp ------------------------------------------
/**
 * Compare two routes towards the same destination
 * - prefer the route with the lowest type (STATIC > IGP > BGP)
 * - prefer the route with the lowest metric
 * - prefer the route with the lowest next-hop address
 */
static int _rt_info_list_cmp(void * item1, void * item2,
			     unsigned int elt_size)
{
  rt_info_t * rtinfo1= *((rt_info_t **) item1);
  rt_info_t * rtinfo2= *((rt_info_t **) item2);

  // Prefer lowest routing protocol type STATIC > IGP > BGP
  if (rtinfo1->type > rtinfo2->type)
    return 1;
  if (rtinfo1->type < rtinfo2->type)
    return -1;

  // Prefer route with lowest metric
  if (rtinfo1->metric > rtinfo2->metric)
    return 1;
  if (rtinfo1->metric < rtinfo2->metric)
    return -1;

  // Tie-break: prefer route with lowest next-hop address
  return route_nexthop_compare(rtinfo1->next_hop, rtinfo2->next_hop);
}

// ----- _rt_info_list_dst ------------------------------------------
static void _rt_info_list_dst(void * item)
{
  rt_info_t * rtinfo= *((rt_info_t **) item);
  net_route_info_destroy(&rtinfo);
}

// -----[ _rt_info_list_create ]-------------------------------------
static inline rt_info_list_t * _rt_info_list_create()
{
  return (rt_info_list_t *) ptr_array_create(ARRAY_OPTION_SORTED|
					     ARRAY_OPTION_UNIQUE,
					     _rt_info_list_cmp,
					     _rt_info_list_dst);
}

// -----[ _rt_info_list_destroy ]------------------------------------
static inline void _rt_info_list_destroy(rt_info_list_t ** list_ref)
{
  ptr_array_destroy((SPtrArray **) list_ref);
}

// -----[ _rt_info_list_length ]-------------------------------------
static inline unsigned int _rt_info_list_length(rt_info_list_t * list)
{
  return ptr_array_length((SPtrArray *) list);
}

// -----[ _rt_info_list_dump ]----------------------------------------
static inline void _rt_info_list_dump(SLogStream * stream,
				      ip_pfx_t prefix,
				      rt_info_list_t * list)
{
  unsigned int index;

  if (_rt_info_list_length(list) == 0) {
    LOG_ERR_ENABLED(LOG_LEVEL_FATAL) {
      log_printf(pLogErr, "\033[1;31mERROR: empty info-list for ");
      ip_prefix_dump(pLogErr, prefix);
      log_printf(pLogErr, "\033[0m\n");
    }
    abort();
  }

  for (index= 0; index < _rt_info_list_length(list); index++) {
    net_route_info_dump(stream, (rt_info_t *) list->data[index]);
    log_printf(stream, "\n");
  }
}

// -----[ _rt_info_list_add ]----------------------------------------
/**
 * Add a new route into the info-list (an info-list corresponds to a
 * destination prefix).
 *
 * Result:
 * - ESUCCESS          if the insertion succeeded
 * - ENET_RT_DUPLICATE if the route could not be added (it already
 *   exists)
 */
static inline int _rt_info_list_add(rt_info_list_t * list,
				    rt_info_t * rtinfo)
{
  if (ptr_array_add((SPtrArray *) list,
		    &rtinfo) < 0)
    return ENET_RT_DUPLICATE;
  return ESUCCESS;
}

// ----- rt_info_list_filter_t --------------------------------------
/**
 * This structure defines a filter to remove routes from the routing
 * table. The possible criteria are as follows:
 * - prefix  : exact match of destination prefix (NULL => match any)
 * - iface   : exact match of outgoing link (NULL => match any)
 * - gateway : exact match of gateway (NULL => match any)
 * - type    : exact match of type (mandatory)
 * All the given criteria have to match a route in order for it to be
 * deleted.
 */
typedef struct {
  ip_pfx_t         * prefix;
  net_iface_t      * oif;
  net_addr_t       * gateway;
  net_route_type_t   type;
  SPtrArray        * del_list;
} rt_info_list_filter_t;

// ----- _net_route_info_matches_filter -----------------------------
/**
 * Check if the given route-info matches the given filter. The
 * following fields are checked:
 * - next-hop interface
 * - gateway address
 * - type (routing protocol)
 *
 * Return:
 * 0 if the route does not match filter
 * 1 if the route matches the filter
 */
static inline
int _net_route_info_matches_filter(rt_info_t * rtinfo,
				   rt_info_list_filter_t * filter)
{
  
  if ((filter->oif != NULL) &&
      (filter->oif != rtinfo->next_hop.oif))
    return 0;

  if ((filter->gateway != NULL) &&
      (*filter->gateway != rtinfo->next_hop.gateway))
    return 0;
  if (filter->type != rtinfo->type)
    return 0;

  return 1;
}

// -----[ _net_route_info_dump_filter ]------------------------------
/**
 * Dump a route-info filter.
 */
/*static inline void
net_route_info_dump_filter(SLogStream * stream,
			   rt_info_list_filter_t * filter)
{
  // Destination filter
  log_printf(stream, "prefix : ");
  if (filter->prefix != NULL)
    ip_prefix_dump(stream, *filter->prefix);
  else
    log_printf(stream, "*");

  // Next-hop address (gateway)
  log_printf(stream, ", gateway: ");
  if (filter->gateway != NULL)
    ip_address_dump(stream, *filter->gateway);
  else
    log_printf(stream, "*");

  // Next-hop interface
  log_printf(stream, ", iface   : ");
  if (filter->oif != NULL)
    ip_address_dump(stream, filter->oif->tIfaceAddr);
    //net_iface_dump_id(stream, filter->oif);
  else
    log_printf(stream, "*");

  // Route type (routing protocol)
  log_printf(stream, ", type: ");
  net_route_type_dump(stream, filter->type);
  log_printf(stream, "\n");
  }*/

// ----- net_info_prefix_dst ----------------------------------------
void net_info_prefix_dst(void * item)
{
  ip_prefix_destroy((ip_pfx_t **) item);
}

// ----- net_info_schedule_removal -----------------------------------
/**
 * This function adds a prefix whose entry in the routing table must
 * be removed. This is used when a route-info-list has been emptied.
 */
static inline void
net_info_schedule_removal(rt_info_list_filter_t * filter,
			  ip_pfx_t * prefix)
{
  if (filter->del_list == NULL)
    filter->del_list= ptr_array_create(0, NULL, net_info_prefix_dst);

  prefix= ip_prefix_copy(prefix);
  ptr_array_append(filter->del_list, prefix);
}

// ----- net_info_removal_for_each ----------------------------------
/**
 *
 */
int net_info_removal_for_each(void * item, void * ctx)
{
  net_rt_t * rt= (net_rt_t *) ctx;
  ip_pfx_t * prefix= *(ip_pfx_t **) item;

  return trie_remove(rt, prefix->tNetwork, prefix->uMaskLen);
}

// -----[ _net_info_removal ]----------------------------------------
/**
 * Remove the scheduled prefixes from the routing table.
 */
static inline int _net_info_removal(rt_info_list_filter_t * filter,
				    net_rt_t * rt)
{
  int result= 0;

  if (filter->del_list != NULL) {
    result= _array_for_each((SArray *) filter->del_list,
			     net_info_removal_for_each, rt);
    ptr_array_destroy(&filter->del_list);
  }
  return result;
}

// -----[ _rt_info_list_del ]----------------------------------------
/**
 * This function removes from the route-list all the routes that match
 * the given attributes: next-hop and route type. A wildcard can be
 * used for the next-hop attribute (by specifying a NULL next-hop
 * link).
 */
static inline int _rt_info_list_del(rt_info_list_t * list,
				    rt_info_list_filter_t * filter,
				    ip_pfx_t * prefix)
{
  unsigned int index;
  rt_info_t * rtinfo;
  unsigned int del_count= 0;
  int result= ENET_RT_UNKNOWN;

  /* Lookup the whole list of routes... */
  index= 0;
  while (index < _rt_info_list_length(list)) {
    rtinfo= (rt_info_t *) list->data[index];

    if (_net_route_info_matches_filter(rtinfo, filter)) {
      ptr_array_remove_at((SPtrArray *) list, index);
      del_count++;
    } else {
      index++;
    }
  }
  
  /* If the list of routes does not contain any route, it should be
     destroyed. Schedule the prefix for removal... */
  if (_rt_info_list_length(list) == 0)
    net_info_schedule_removal(filter, prefix);

  if (del_count > 0)
    result= ESUCCESS;
  return result;
}

// -----[ _rt_info_list_get ]----------------------------------------
static inline rt_info_t *
_rt_info_list_get(rt_info_list_t * list, unsigned int index)
{
  if (index < _rt_info_list_length(list))
    return list->data[index];
  return NULL;
}

// ----- _rt_il_dst -------------------------------------------------
static void _rt_il_dst(void ** ppItem)
{
  _rt_info_list_destroy((rt_info_list_t **) ppItem);
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE (net_rt_t)
//
/////////////////////////////////////////////////////////////////////

// ----- rt_create --------------------------------------------------
/**
 * Create a routing table.
 */
net_rt_t * rt_create()
{
  return (net_rt_t *) trie_create(_rt_il_dst);
}

// ----- rt_destroy -------------------------------------------------
/**
 * Free a routing table.
 */
void rt_destroy(net_rt_t ** rt_ref)
{
  trie_destroy((STrie **) rt_ref);
}

// -----[ rt_find_best ]---------------------------------------------
/**
 * Find the route that best matches the given prefix. If a particular
 * route type is given, returns only the route with the requested
 * type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 */
rt_info_t * rt_find_best(net_rt_t * rt, net_addr_t addr,
			 net_route_type_t type)
{
  rt_info_list_t * list;
  int index;
  rt_info_t * rtinfo;

  /* First, retrieve the list of routes that best match the given
     prefix */
  list= (rt_info_list_t *) trie_find_best((STrie *) rt, addr, 32);

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (list != NULL) {

    assert(_rt_info_list_length(list) != 0);

    for (index= 0; index < _rt_info_list_length(list); index++) {
      rtinfo= _rt_info_list_get(list, index);
      if (rtinfo->type & type)
	return rtinfo;
    }
  }

  return NULL;
}

// -----[ rt_find_exact ]--------------------------------------------
/**
 * Find the route that exactly matches the given prefix. If a
 * particular route type is given, returns only the route with the
 * requested type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 */
rt_info_t * rt_find_exact(net_rt_t * rt, ip_pfx_t prefix,
			  net_route_type_t type)
{
  rt_info_list_t * list;
  int index;
  rt_info_t * rtinfo;

  /* First, retrieve the list of routes that exactly match the given
     prefix */
  list= (rt_info_list_t *)
    trie_find_exact((STrie *) rt, prefix.tNetwork, prefix.uMaskLen);

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (list != NULL) {

    assert(_rt_info_list_length(list) != 0);

    for (index= 0; index < _rt_info_list_length(list); index++) {
      rtinfo= _rt_info_list_get(list, index);
      if (rtinfo->type & type)
	return rtinfo;
    }
  }

  return NULL;
}

// -----[ rt_add_route ]---------------------------------------------
/**
 * Add a route into the routing table.
 *
 * Returns:
 *   ESUCCESS          on success
 *   ENET_RT_DUPLICATE in case of error (duplicate route)
 */
int rt_add_route(net_rt_t * rt, ip_pfx_t prefix,
		 rt_info_t * rtinfo)
{
  rt_info_list_t * list;

  list= (rt_info_list_t *) trie_find_exact((STrie *) rt,
					   prefix.tNetwork,
					   prefix.uMaskLen);

  // Create a new info-list if none exists for the given prefix
  if (list == NULL) {
    list= _rt_info_list_create();
    assert(_rt_info_list_add(list, rtinfo) == ESUCCESS);
    trie_insert((STrie *) rt, prefix.tNetwork, prefix.uMaskLen, list);

  } else {

    return _rt_info_list_add(list, rtinfo);

  }
  return ESUCCESS;
}

// -----[ rt_del_for_each ]------------------------------------------
/**
 * Helper function for the 'rt_del_route' function. Handles the
 * deletion of a single prefix (can be called by
 * 'radix_tree_for_each')
 */
int rt_del_for_each(uint32_t key, uint8_t key_len,
		    void * item, void * ctx)
{
  rt_info_list_t * list= (rt_info_list_t *) item;
  rt_info_list_filter_t * filter= (rt_info_list_filter_t *) ctx;
  ip_pfx_t prefix;
  int result;

  if (list == NULL)
    return -1;

  prefix.tNetwork= key;
  prefix.uMaskLen= key_len;

  /* Remove from the list all the routes that match the given
     attributes */
  result= _rt_info_list_del(list, filter, &prefix);

  /* If we use widlcards, the call never fails, otherwise, it depends
     on the '_rt_info_list_del' call */
  return ((filter->prefix != NULL) &&
	  (filter->oif != NULL) &&
	  (filter->gateway)) ? result : 0;
}

// ----- rt_del_route -----------------------------------------------
/**
 * Remove route(s) from the given routing table. The route(s) to be
 * removed must match the given attributes: prefix, next-hop and
 * type. However, wildcards can be used for the prefix and next-hop
 * attributes. The NULL value corresponds to the wildcard.
 */
int rt_del_route(net_rt_t * rt, ip_pfx_t * prefix,
		 net_iface_t * oif, net_addr_t * gateway,
		 net_route_type_t type)
{
  rt_info_list_t * list;
  rt_info_list_filter_t filter;
  int result;

  /* Prepare the attributes of the routes to be removed (context) */
  filter.prefix= prefix;
  filter.oif= oif;
  filter.gateway= gateway;
  filter.type= type;
  filter.del_list= NULL;

  /* Prefix specified ? or wildcard ? */
  if (prefix != NULL) {

    /* Get the list of routes towards the given prefix */
    list= (rt_info_list_t *) trie_find_exact((STrie *) rt,
					     prefix->tNetwork,
					     prefix->uMaskLen);
    result= rt_del_for_each(prefix->tNetwork, prefix->uMaskLen,
			    list, &filter);

  } else {

    /* Remove all the routes that match the given attributes, whatever
       the prefix is */
    result= trie_for_each((STrie *) rt, rt_del_for_each, &filter);

  }

  _net_info_removal(&filter, rt);

  return result;
}

typedef struct {
  FRadixTreeForEach   fForEach;
  void              * ctx;
} _for_each_ctx_t;

// ----- _rt_for_each_function --------------------------------------
/**
 * Helper function for the 'rt_for_each' function. This function
 * execute the callback function for each entry in the info-list of
 * the current prefix.
 */
static int _rt_for_each_function(uint32_t key, uint8_t key_len,
				 void * item, void * ctx)
{
  _for_each_ctx_t * for_each_ctx= (_for_each_ctx_t *) ctx;
  rt_info_list_t * list= (rt_info_list_t *) item;
  unsigned int index;

  for (index= 0; index < _rt_info_list_length(list); index++) {
    if (for_each_ctx->fForEach(key, key_len, list->data[index],
			       for_each_ctx->ctx) != 0)
      return -1;
  }
  return 0;
}

// ----- rt_for_each ------------------------------------------------
/**
 * Execute the given function for each entry (prefix, type) in the
 * routing table.
 */
int rt_for_each(net_rt_t * rt, FRadixTreeForEach fForEach,
		void * ctx)
{
  _for_each_ctx_t for_each_ctx;

  for_each_ctx.fForEach= fForEach;
  for_each_ctx.ctx= ctx;

  return trie_for_each((STrie *) rt, _rt_for_each_function, &for_each_ctx);
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ _route_nexthop_dump ]--------------------------------------
/**
 *
 */
static inline void _route_nexthop_dump(SLogStream * stream,
				       rt_entry_t rtentry)
{
  ip_address_dump(stream, rtentry.gateway);
  log_printf(stream, "\t");
  ip_address_dump(stream, rtentry.oif->tIfaceAddr);
  //net_iface_dump_id(stream, rtentry.oif);
}

// ----- net_route_type_dump ----------------------------------------
/**
 *
 */
void net_route_type_dump(SLogStream * stream, net_route_type_t type)
{
  switch (type) {
  case NET_ROUTE_DIRECT:
    log_printf(stream, "DIRECT");
    break;
  case NET_ROUTE_STATIC:
    log_printf(stream, "STATIC");
    break;
  case NET_ROUTE_IGP:
    log_printf(stream, "IGP");
    break;
  case NET_ROUTE_BGP:
    log_printf(stream, "BGP");
    break;
  default:
    abort();
  }
}

// -----[ net_route_info_dump ]--------------------------------------
/**
 * Output format:
 * <dst-prefix> <link/if> <weight> <type> [ <state> ]
 */
void net_route_info_dump(SLogStream * stream, rt_info_t * rtinfo)
{
  ip_prefix_dump(stream, rtinfo->prefix);
  log_printf(stream, "\t");
  _route_nexthop_dump(stream, rtinfo->next_hop);
  log_printf(stream, "\t%u\t", rtinfo->metric);
  net_route_type_dump(stream, rtinfo->type);
  if (!net_iface_is_enabled(rtinfo->next_hop.oif)) {
    log_printf(stream, "\t[DOWN]");
  }
}

// -----[ _rt_dump_for_each ]----------------------------------------
static int _rt_dump_for_each(uint32_t key, uint8_t key_len,
			     void * item, void * ctx)
{
  SLogStream * stream= (SLogStream *) ctx;
  rt_info_list_t * list= (rt_info_list_t *) item;
  ip_pfx_t pfx;
  
  pfx.tNetwork= key;
  pfx.uMaskLen= key_len;
  
  _rt_info_list_dump(stream, pfx, list);
  return 0;
}

// -----[ rt_dump ]--------------------------------------------------
/**
 * Dump the routing table for the given destination. The destination
 * can be of type NET_DEST_ANY, NET_DEST_ADDRESS and NET_DEST_PREFIX.
 */
void rt_dump(SLogStream * stream, net_rt_t * rt, SNetDest dest)
{
  rt_info_t * rtinfo;

  //fprintf(pStream, "DESTINATION\tNEXTHOP\tIFACE\tMETRIC\tTYPE\n");

  switch (dest.tType) {

  case NET_DEST_ANY:
    trie_for_each((STrie *) rt, _rt_dump_for_each, stream);
    break;

  case NET_DEST_ADDRESS:
    rtinfo= rt_find_best(rt, dest.uDest.sPrefix.tNetwork, NET_ROUTE_ANY);
    if (rtinfo != NULL) {
      net_route_info_dump(stream, rtinfo);
      log_printf(stream, "\n");
    }
    break;

  case NET_DEST_PREFIX:
    rtinfo= rt_find_exact(rt, dest.uDest.sPrefix, NET_ROUTE_ANY);
    if (rtinfo != NULL) {
      net_route_info_dump(stream, rtinfo);
      log_printf(stream, "\n");
    }
    break;

  default:
    abort();
  }
}

