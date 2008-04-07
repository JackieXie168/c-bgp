// ==================================================================
// @(#)iface.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/08/2003
// $Id: iface.c,v 1.3 2008-04-07 09:31:46 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/memory.h>

#include <net/iface.h>
#include <net/iface_rtr.h>
#include <net/iface_ptp.h>
#include <net/iface_ptmp.h>
#include <net/link.h>
#include <net/net_types.h>
#include <net/node.h>
#include <util/str_format.h>

typedef struct {
  char * name;
  char * short_name;
} net_iface_name_t;

static net_iface_name_t IFACE_TYPES[NET_IFACE_MAX]= {
  { "loopback",            "lo" },
  { "router-to-router",    "rtr" },
  { "point-to-point",      "ptp" },
  { "point-to-multipoint", "ptmp" },
  { "virtual",             "tun" },
};

// -----[ net_iface_type2name ]--------------------------------------
static char * net_iface_type2name(net_iface_type_t type, int short_name)
{
  if (short_name)
    return IFACE_TYPES[type].short_name;
  return IFACE_TYPES[type].name;
}

// -----[ _iface_recv ]----------------------------------------------
static int _iface_recv(net_iface_t *  iface, net_msg_t * msg) {
  return node_recv_msg(iface->src_node, iface, msg);
}

// -----[ net_iface_new ]--------------------------------------------
net_iface_t * net_iface_new(net_node_t * node,
			    net_iface_type_t type)
{
  net_iface_t * iface= (net_iface_t *) MALLOC(sizeof(net_iface_t));
  iface->src_node= node;
  iface->type= type;
  iface->tIfaceAddr= 0;
  iface->tIfaceMask= 0;
  iface->connected= 0;
  iface->flags= NET_LINK_FLAG_UP;
  iface->user_data= NULL;

  /* Operations */
  iface->ops.send= NULL;
  iface->ops.recv= _iface_recv;
  iface->ops.destroy= NULL;

  /* Physical attributes */
  iface->phys.delay= 0;
  iface->phys.capacity= 0;
  iface->phys.load= 0;

  /* IGP attributes */
  iface->pWeights= NULL;
  iface->pWeights= net_igp_weights_create(1/*tDepth*/);

#ifdef OSPF_SUPPORT
  iface->tArea= OSPF_NO_AREA;
#endif
  
  return iface;
}

// -----[ _iface_new_loopback ]--------------------------------------
static net_iface_t * _iface_new_loopback(net_node_t * node, net_addr_t addr)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_LOOPBACK);
  iface->tIfaceAddr= addr;
  iface->tIfaceMask= 32;
  return iface;
}

// -----[ _iface_new_virtual ]---------------------------------------
static net_iface_t * _iface_new_virtual(net_node_t * node, net_addr_t addr)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_VIRTUAL);
  iface->tIfaceAddr= addr;
  iface->tIfaceMask= 32;
  return iface;
}

// -----[ net_iface_destroy ]----------------------------------------
void net_iface_destroy(net_iface_t ** iface_ref)
{
  if (*iface_ref != NULL) {
    FREE(*iface_ref);
    *iface_ref= NULL;
  }    
}

// -----[ net_iface_connect_iface ]----------------------------------
/**
 * Connect an interface to another one.
 */
int net_iface_connect_iface(net_iface_t * iface, net_iface_t * dst)
{
  // Check that this interface is not connected
  if (iface->connected != 0)
    return ENET_IFACE_CONNECTED;

  // Check that the dst interface is not connected or connected
  // to this interface
  if ((dst->connected != 0) &&
      (dst->dest.iface != iface))
    return ENET_IFACE_CONNECTED;

  // Check that interface types are equal
  if (iface->type != dst->type)
    return ENET_IFACE_INCOMPATIBLE;

  // Check that interface type is RTR or PTP
  if ((iface->type != NET_IFACE_RTR) &&
      (iface->type != NET_IFACE_PTP))
    return ENET_IFACE_INCOMPATIBLE;

  // Check that endpoint addresses are different
  if ((iface->tIfaceAddr == dst->tIfaceAddr) ||
      (iface->src_node == dst->src_node))
    return ENET_LINK_LOOP;

  iface->dest.iface= dst;
  iface->connected= 1;
  return ESUCCESS;
}

// -----[ net_iface_connect_subnet ]---------------------------------
/**
 * Connect an interface to a subnet.
 */
int net_iface_connect_subnet(net_iface_t * iface, net_subnet_t * dst)
{
  // Check that interface is not yet connected
  if (iface->connected != 0)
    return ENET_IFACE_CONNECTED;

  // Check that interface type is PTMP
  if (iface->type != NET_IFACE_PTMP)
    return ENET_IFACE_INCOMPATIBLE;
    
  iface->dest.subnet= dst;
  iface->connected= 1;
  return ESUCCESS;
}

// -----[ net_iface_disconnect ]-----------------------------------
int net_iface_disconnect(net_iface_t * iface)
{
  iface->connected= 0;
  return ESUCCESS;
}

// -----[ net_iface_has_address ]------------------------------------
int net_iface_has_address(net_iface_t * iface, net_addr_t addr)
{
  // Router-to-Router link does not own interface
  if (iface->type == NET_IFACE_RTR)
    return 0;

  if (iface->tIfaceAddr == addr)
    return 1;
  return 0;
}

// -----[ net_iface_has_prefix ]-------------------------------------
int net_iface_has_prefix(net_iface_t * iface, SPrefix sPrefix)
{
  SPrefix sIfacePrefix;
  if (iface->type == NET_IFACE_RTR)
    return 0;

  sIfacePrefix= net_iface_dst_prefix(iface);
  return (ip_prefix_cmp(&sIfacePrefix, &sPrefix) == 0);
}

// -----[ net_iface_src_address ]------------------------------------
net_addr_t net_iface_src_address(net_iface_t * iface)
{
  if (iface->type == NET_IFACE_RTR)
    return iface->src_node->tAddr;
  return iface->tIfaceAddr;
}

// -----[ net_iface_dst_prefix ]-------------------------------------
SPrefix net_iface_dst_prefix(net_iface_t * iface)
{
  return net_iface_id(iface);
}

// -----[ net_iface_id ]---------------------------------------------
net_iface_id_t net_iface_id(net_iface_t * iface)
{
  net_iface_id_t tID;
  tID.tNetwork= iface->tIfaceAddr;
  tID.uMaskLen= iface->tIfaceMask;
  return tID;
}

// -----[ net_iface_is_connected ]-----------------------------------
int net_iface_is_connected(net_iface_t * iface)
{
  return iface->connected;
}

// -----[ net_iface_factory ]----------------------------------------
/**
 * This function creates an interface based on the given interface
 * identifier (IP address or IP prefix) and interface type.
 */
int net_iface_factory(net_node_t * node, net_iface_id_t id,
		      net_iface_type_t type, net_iface_t ** iface_ref)
{
  switch (type) {
  case NET_IFACE_LOOPBACK:
    if (id.uMaskLen != 32)
      return ENET_IFACE_INVALID_ID;
    *iface_ref= _iface_new_loopback(node, id.tNetwork);
    break;

  case NET_IFACE_RTR:
    if (id.uMaskLen != 32)
      return ENET_IFACE_INVALID_ID;
    *iface_ref= net_iface_new_rtr(node, id.tNetwork);
    break;

  case NET_IFACE_PTP:
    if (id.uMaskLen >= 32)
      return ENET_IFACE_INVALID_ID;
    *iface_ref= net_iface_new_ptp(node, id);
    break;

  case NET_IFACE_PTMP:
    if (id.uMaskLen >= 32)
      return ENET_IFACE_INVALID_ID;
    *iface_ref= net_iface_new_ptmp(node, id);
    break;

  case NET_IFACE_VIRTUAL:
    if (id.uMaskLen != 32)
      return ENET_IFACE_INVALID_ID;
    *iface_ref= _iface_new_virtual(node, id.tNetwork);
    break;

  default:
    return ENET_IFACE_INVALID_TYPE;
  }

  return ESUCCESS;
}

// -----[ _net_iface_is_bidir ]---------------------------------------
static inline int _net_iface_is_bidir(net_iface_t * iface)
{
  switch (iface->type) {
  case NET_IFACE_LOOPBACK:
  case NET_IFACE_PTMP:
  case NET_IFACE_VIRTUAL:
    return 0;
  case NET_IFACE_RTR:
  case NET_IFACE_PTP:
    return 1;
  default:
    abort();
  }
}

// -----[ _net_iface_get_reverse ]-----------------------------------
static inline net_error_t
_net_iface_get_reverse(net_iface_t * iface, net_iface_t ** rev_iface_ref)
{
  if (!_net_iface_is_bidir(iface))
    return ENET_IFACE_NOT_SUPPORTED;
  if (!net_iface_is_connected(iface))
    return ENET_IFACE_NOT_CONNECTED;
  *rev_iface_ref= iface->dest.iface;
  return ESUCCESS;
}

// -----[ net_iface_recv ]-------------------------------------------
int net_iface_recv(net_iface_t * iface, net_msg_t * msg)
{
  assert(iface->ops.recv != NULL);
  return iface->ops.recv(iface, msg);
}

// -----[ net_iface_send ]-------------------------------------------
int net_iface_send(net_iface_t * iface, net_addr_t l2_addr,
		   net_msg_t * msg)
{
  assert(iface->ops.send != NULL);
  
  // Check link's state
  if (!net_iface_is_connected(iface) ||
      !net_iface_is_enabled(iface)) {
    network_drop(msg);
    return ENET_LINK_DOWN;
  }
  
  return iface->ops.send(iface, l2_addr, msg);
}


/////////////////////////////////////////////////////////////////////
// ATTRIBUTES
/////////////////////////////////////////////////////////////////////

// -----[ _net_iface_set_flag ]--------------------------------------
static inline void _net_iface_set_flag(net_iface_t * iface,
				       uint8_t flag, int state)
{
  if (state)
    iface->flags|= flag;
  else
    iface->flags&= ~flag;
}

// -----[ _net_iface_get_flag ]--------------------------------------
static inline int _net_iface_get_flag(net_iface_t * iface, uint8_t flag)
{
  return (iface->flags & flag) != 0;
}

// -----[ net_iface_is_enabled ]-------------------------------------
int net_iface_is_enabled(net_iface_t * iface)
{
  return _net_iface_get_flag(iface, NET_LINK_FLAG_UP);
}

// -----[ net_iface_set_enabled ]------------------------------------
void net_iface_set_enabled(net_iface_t * iface, int enabled)
{
  _net_iface_set_flag(iface, NET_LINK_FLAG_UP, enabled);
}

// -----[ net_iface_get_metric ]-------------------------------------
/**
 * To-do: if (tTOS > depth) => return NET_IGP_MAX_WEIGHT
 */
net_igp_weight_t net_iface_get_metric(net_iface_t * iface, net_tos_t tos)
{
  if (iface->pWeights == NULL)
    return NET_IGP_MAX_WEIGHT;
  return iface->pWeights->data[tos];
}

// -----[ net_iface_set_metric ]-------------------------------------
/**
 * To-do: return error if (pWeights == NULL) or if (depth <= tTOS)
 */
int net_iface_set_metric(net_iface_t * iface, net_tos_t tos,
			 net_igp_weight_t weight, net_iface_dir_t dir)
{
  net_error_t error;
  net_iface_t * rev_iface;

  assert(iface->pWeights != NULL);
  assert(tos < net_igp_weights_depth(iface->pWeights));

  if (dir == BIDIR) {
    error= _net_iface_get_reverse(iface, &rev_iface);
    if (error != ESUCCESS)
      return error;
    error= net_iface_set_metric(rev_iface, tos, weight, UNIDIR);
    if (error != ESUCCESS)
      return error;
  }

  iface->pWeights->data[tos]= weight;
  return ESUCCESS;
}

// -----[ net_iface_get_delay ]--------------------------------------
net_link_delay_t net_iface_get_delay(net_iface_t * iface)
{
  return iface->phys.delay;
}

// -----[ net_iface_set_delay ]--------------------------------------
net_error_t net_iface_set_delay(net_iface_t * iface,
				net_link_delay_t delay,
				net_iface_dir_t dir)
{
  net_iface_t * rev_iface;
  net_error_t error;
  if (dir == BIDIR) {
    error= _net_iface_get_reverse(iface, &rev_iface);
    if (error != ESUCCESS)
      return error;
    error= net_iface_set_delay(rev_iface, delay, UNIDIR);
    if (error != ESUCCESS)
      return error;
  }
  iface->phys.delay= delay;
  return ESUCCESS;
}

// -----[ net_iface_set_capacity ]-----------------------------------
net_error_t net_iface_set_capacity(net_iface_t * iface,
				   net_link_load_t capacity,
				   net_iface_dir_t dir)
{
  net_iface_t * rev_iface;
  net_error_t error;
  if (dir == BIDIR) {
    error= _net_iface_get_reverse(iface, &rev_iface);
    if (error != ESUCCESS)
      return error;
    error= net_iface_set_capacity(rev_iface, capacity, UNIDIR);
    if (error != ESUCCESS)
      return error;
  }
  iface->phys.capacity= capacity;
  return ESUCCESS;
}

// -----[ net_iface_get_capacity ]-----------------------------------
net_link_load_t net_iface_get_capacity(net_iface_t * iface)
{
  return iface->phys.capacity;
}

// -----[ net_iface_set_load ]---------------------------------------
void net_iface_set_load(net_iface_t * iface, net_link_load_t load)
{
  iface->phys.load= load;
}

// -----[ net_iface_get_load ]---------------------------------------
net_link_load_t net_iface_get_load(net_iface_t * iface)
{
  return iface->phys.load;
}

// -----[ net_iface_add_load ]---------------------------------------
void net_iface_add_load(net_iface_t * iface, net_link_load_t inc_load)
{
  uint64_t big_load= iface->phys.load;
  big_load+= inc_load;
  if (big_load > NET_LINK_MAX_LOAD)
    iface->phys.load= NET_LINK_MAX_LOAD;
  else
    iface->phys.load= (net_link_load_t) big_load;
}


/////////////////////////////////////////////////////////////////////
// DUMP
/////////////////////////////////////////////////////////////////////

// -----[ net_iface_dump_type ]--------------------------------------
void net_iface_dump_type(SLogStream * stream, net_iface_t * iface)
{
  char * name= net_iface_type2name(iface->type, 1);
  assert(name != NULL);
  log_printf(stream, "%s", name);
}

// -----[ net_iface_dump_id ]----------------------------------------
void net_iface_dump_id(SLogStream * stream, net_iface_t * iface)
{
  ip_prefix_dump(stream, net_iface_id(iface));
}

// -----[ net_iface_dump_dest ]--------------------------------------
void net_iface_dump_dest(SLogStream * stream, net_iface_t * iface)
{
  if (!iface->connected) {
    log_printf(stream, "not-connected");
    return;
  }
  switch (iface->type) {
  case NET_IFACE_RTR:
    ip_address_dump(stream, iface->tIfaceAddr);
    break;
  case NET_IFACE_PTP:
    net_iface_dump_id(stream, iface->dest.iface);
    log_printf(stream, "\t");
    ip_address_dump(stream, iface->dest.iface->src_node->tAddr);
    break;
  case NET_IFACE_PTMP:
    ip_prefix_dump(stream, iface->dest.subnet->sPrefix);
    break;
  case NET_IFACE_VIRTUAL:
    ip_address_dump(stream, iface->dest.end_point);
  default:
    abort();
  }
}

// -----[ net_iface_dump ]-------------------------------------------
void net_iface_dump(SLogStream * stream, net_iface_t * iface, int with_dest)
{
  net_iface_dump_type(stream, iface);
  log_printf(stream, "\t");
  net_iface_dump_id(stream, iface);
  if (with_dest) {
    log_printf(stream, "\t");
    net_iface_dump_dest(stream, iface);
  }
}

// -----[ net_iface_dumpf ]------------------------------------------
/**
 * Specification  | Field
 * ---------------+------------------------
 * %D             | destination
 * %I             | identifier
 * %c             | capacity
 * %d             | delay
 * %l             | load
 * %s             | state (up/down)
 * %w             | IGP weight
 */
static int _net_iface_dumpf(SLogStream * stream, void * ctx,
			    char format)
{
  net_iface_t * iface= (net_iface_t *) ctx;
  switch (format) {
  case 'D':
    net_iface_dump_dest(stream, iface);
    break;
  case 'I':
    net_iface_dump_id(stream, iface);
    break;
  case 'c':
    log_printf(stream, "%u", iface->phys.capacity);
    break;
  case 'd':
    log_printf(stream, "%u", iface->phys.delay);
    break;
  case 'l':
    log_printf(stream, "%u", iface->phys.load);
    break;
  case 's':
    if (_net_iface_get_flag(iface, NET_LINK_FLAG_UP))
      log_printf(stream, "UP");
    else
      log_printf(stream, "DOWN");
    break;
  case 'w':
    if (iface->pWeights != NULL)
      log_printf(stream, "%u", iface->pWeights->data[0]);
    else
      log_printf(stream, "undef");
    break;
  default:
    return -1;
  }
  return 0;
}

// -----[ net_iface_dumpf ]------------------------------------------
void net_iface_dumpf(SLogStream * stream, net_iface_t * iface,
		     const char * format)
{
  assert(str_format_for_each(stream, _net_iface_dumpf, iface, format) == 0);
}



/////////////////////////////////////////////////////////////////////
// STRING TO TYPE / IDENTIFIER CONVERSION
/////////////////////////////////////////////////////////////////////

// -----[ net_iface_str2type ]---------------------------------------
net_error_t net_iface_str2type(char * str, net_iface_type_t * ptr_type)
{
  net_iface_type_t type= 0;
  while (type < NET_IFACE_MAX) {
    if (!strcmp(str, IFACE_TYPES[type].name) ||
	!strcmp(str, IFACE_TYPES[type].short_name)) {
      *ptr_type= type;
      return ESUCCESS;
    }
    type++;
  }
  return ENET_IFACE_INVALID_TYPE;
}

// -----[ net_iface_str2id ]-----------------------------------------
/**
 * Convert a string to an interface identifier (ID). The string can
 * be an IP address or a prefix.
 */
net_error_t net_iface_str2id(char * str, net_iface_id_t * ptr_id)
{
  SNetDest id;

  // Destination must be an IP address or an IP prefix
  if ((ip_string_to_dest(str, &id) < 0) ||
      (id.tType == NET_DEST_ANY) ||
      (id.tType == NET_DEST_INVALID)) {
    return ENET_IFACE_INVALID_ID;
  }

  if (((id.tType == NET_DEST_ADDRESS) &&
       (id.uDest.tAddr == NET_ADDR_ANY)) ||
      ((id.tType == NET_DEST_PREFIX) &&
       (id.uDest.sPrefix.tNetwork == NET_ADDR_ANY)))
    return ENET_IFACE_INVALID_ID;

  // Convert destination to prefix (interface ID)
  *ptr_id= net_dest_to_prefix(id);
  return ESUCCESS;
}
