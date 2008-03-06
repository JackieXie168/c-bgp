// ==================================================================
// @(#)iface.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/08/2003
// @lastdate 05/03/2008
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
  char *           pcName;
  char *           pcShortName;
  net_iface_type_t tType;
} SNetIfaceName;
static SNetIfaceName IFACE_TYPES[]= {
  { "loopback",            "lo",   NET_IFACE_LOOPBACK },
  { "router-to-router",    "rtr",  NET_IFACE_RTR },
  { "point-to-point",      "ptp",  NET_IFACE_PTP },
  { "point-to-multipoint", "ptmp", NET_IFACE_PTMP },
  { "virtual",             "tun",  NET_IFACE_VIRTUAL },
};
#define IFACE_NUM_TYPES sizeof(IFACE_TYPES)/sizeof(SNetIfaceName)

// -----[ net_iface_type2name ]--------------------------------------
static char * net_iface_type2name(net_iface_type_t tType, int iShort)
{
  unsigned int uIndex= 0;
  while (uIndex < IFACE_NUM_TYPES) {
    if (IFACE_TYPES[uIndex].tType == tType) {
      return (iShort ?
	      IFACE_TYPES[uIndex].pcShortName :
	      IFACE_TYPES[uIndex].pcName);
    }
    uIndex++;
  }
  return NULL;
}

// -----[ _iface_recv ]----------------------------------------------
static int _iface_recv(SNetIface *  pIface, SNetMessage * pMsg) {
  return node_recv_msg(pIface->pSrcNode, pIface, pMsg);
}

// -----[ net_iface_new ]--------------------------------------------
SNetIface * net_iface_new(SNetNode * pNode,
			  net_iface_type_t tType)
{
  SNetIface * pIface= (SNetIface *) MALLOC(sizeof(SNetIface));
  pIface->pSrcNode= pNode;
  pIface->tType= tType;
  pIface->tIfaceAddr= 0;
  pIface->tIfaceMask= 0;
  pIface->uConnected= 0;
  pIface->pWeights= NULL;

  pIface->pWeights= net_igp_weights_create(1/*tDepth*/);
  pIface->tDelay= 0;
  pIface->tCapacity= 0;
  pIface->tLoad= 0;
  pIface->uFlags= NET_LINK_FLAG_UP;
  pIface->pContext= NULL;
  pIface->fSend= NULL;
  pIface->fRecv= _iface_recv;
  pIface->fDestroy= NULL;
#ifdef OSPF_SUPPORT
  pIface->tArea= OSPF_NO_AREA;
#endif
  
  return pIface;
}

// -----[ _iface_new_loopback ]--------------------------------------
static SNetIface * _iface_new_loopback(SNetNode * pNode, net_addr_t tAddr)
{
  SNetIface * pIface= net_iface_new(pNode, NET_IFACE_LOOPBACK);
  pIface->tIfaceAddr= tAddr;
  pIface->tIfaceMask= 32;
  return pIface;
}

// -----[ _iface_new_virtual ]---------------------------------------
static SNetIface * _iface_new_virtual(SNetNode * pNode, net_addr_t tAddr)
{
  SNetIface * pIface= net_iface_new(pNode, NET_IFACE_VIRTUAL);
  pIface->tIfaceAddr= tAddr;
  pIface->tIfaceMask= 32;
  return pIface;
}

// -----[ net_iface_destroy ]----------------------------------------
void net_iface_destroy(SNetIface ** ppIface)
{
  if (*ppIface != NULL) {
    FREE(*ppIface);
    *ppIface= NULL;
  }    
}

// -----[ net_iface_connect_iface ]----------------------------------
/**
 * Connect an interface to another one.
 */
int net_iface_connect_iface(SNetIface * pIface, SNetIface * pDst)
{
  // Check that this interface is not connected
  if (pIface->uConnected != 0)
    return NET_ERROR_MGMT_IFACE_CONNECTED;

  // Check that the dst interface is not connected or connected
  // to this interface
  if ((pDst->uConnected != 0) &&
      (pDst->tDest.pIface != pIface))
    return NET_ERROR_MGMT_IFACE_CONNECTED;

  // Check that interface types are equal
  if (pIface->tType != pDst->tType)
    return NET_ERROR_MGMT_IFACE_INCOMPATIBLE;

  // Check that interface type is RTR or PTP
  if ((pIface->tType != NET_IFACE_RTR) &&
      (pIface->tType != NET_IFACE_PTP))
    return NET_ERROR_MGMT_IFACE_INCOMPATIBLE;

  // Check that endpoint addresses are different
  if ((pIface->tIfaceAddr == pDst->tIfaceAddr) ||
      (pIface->pSrcNode == pDst->pSrcNode))
    return NET_ERROR_MGMT_LINK_LOOP;

  pIface->tDest.pIface= pDst;
  pIface->uConnected= 1;
  return NET_SUCCESS;
}

// -----[ net_iface_connect_subnet ]---------------------------------
/**
 * Connect an interface to a subnet.
 */
int net_iface_connect_subnet(SNetIface * pIface, SNetSubnet * pDst)
{
  // Check that interface is not yet connected
  if (pIface->uConnected != 0)
    return NET_ERROR_MGMT_IFACE_CONNECTED;

  // Check that interface type is PTMP
  if (pIface->tType != NET_IFACE_PTMP)
    return NET_ERROR_MGMT_IFACE_INCOMPATIBLE;
    
  pIface->tDest.pSubnet= pDst;
  pIface->uConnected= 1;
  return NET_SUCCESS;
}

// -----[ net_iface_disconnect ]-----------------------------------
int net_iface_disconnect(SNetIface * pIface)
{
  pIface->uConnected= 0;
  return NET_SUCCESS;
}

// -----[ net_iface_has_address ]------------------------------------
int net_iface_has_address(SNetIface * pIface, net_addr_t tAddr)
{
  // Router-to-Router link does not own interface
  if (pIface->tType == NET_IFACE_RTR)
    return 0;

  if (pIface->tIfaceAddr == tAddr)
    return 1;
  return 0;
}

// -----[ net_iface_has_prefix ]-------------------------------------
int net_iface_has_prefix(SNetIface * pIface, SPrefix sPrefix)
{
  SPrefix sIfacePrefix;
  if (pIface->tType == NET_IFACE_RTR)
    return 0;

  sIfacePrefix= net_iface_dst_prefix(pIface);
  return (ip_prefix_cmp(&sIfacePrefix, &sPrefix) == 0);
}

// -----[ net_iface_src_address ]------------------------------------
net_addr_t net_iface_src_address(SNetIface * pIface)
{
  if (pIface->tType == NET_IFACE_RTR)
    return pIface->pSrcNode->tAddr;
  return pIface->tIfaceAddr;
}

// -----[ net_iface_dst_prefix ]-------------------------------------
SPrefix net_iface_dst_prefix(SNetIface * pIface)
{
  return net_iface_id(pIface);
}

// -----[ net_iface_id ]---------------------------------------------
net_iface_id_t net_iface_id(SNetIface * pIface)
{
  net_iface_id_t tID;
  tID.tNetwork= pIface->tIfaceAddr;
  tID.uMaskLen= pIface->tIfaceMask;
  return tID;
}

// -----[ net_iface_is_connected ]-----------------------------------
int net_iface_is_connected(SNetIface * pIface)
{
  return pIface->uConnected;
}

// -----[ net_iface_factory ]----------------------------------------
/**
 * This function creates an interface based on the given interface
 * identifier (IP address or IP prefix) and interface type.
 */
int net_iface_factory(SNetNode * pNode, net_iface_id_t tID,
		      net_iface_type_t tType, SNetIface ** ppIface)
{
  switch (tType) {
  case NET_IFACE_LOOPBACK:
    if (tID.uMaskLen != 32)
      return NET_ERROR_MGMT_INVALID_IFACE_ID;
    *ppIface= _iface_new_loopback(pNode, tID.tNetwork);
    break;

  case NET_IFACE_RTR:
    if (tID.uMaskLen != 32)
      return NET_ERROR_MGMT_INVALID_IFACE_ID;
    *ppIface= net_iface_new_rtr(pNode, tID.tNetwork);
    break;

  case NET_IFACE_PTP:
    if (tID.uMaskLen >= 32)
      return NET_ERROR_MGMT_INVALID_IFACE_ID;
    *ppIface= net_iface_new_ptp(pNode, tID);
    break;

  case NET_IFACE_PTMP:
    if (tID.uMaskLen >= 32)
      return NET_ERROR_MGMT_INVALID_IFACE_ID;
    *ppIface= net_iface_new_ptmp(pNode, tID);
    break;

  case NET_IFACE_VIRTUAL:
    if (tID.uMaskLen != 32)
      return NET_ERROR_MGMT_INVALID_IFACE_ID;
    *ppIface= _iface_new_virtual(pNode, tID.tNetwork);
    break;

  default:
    return NET_ERROR_MGMT_INVALID_IFACE_TYPE;
  }

  return NET_SUCCESS;
}

// -----[ _net_iface_is_bidir ]---------------------------------------
static inline int _net_iface_is_bidir(SNetIface * pIface)
{
  switch (pIface->tType) {
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
static inline int _net_iface_get_reverse(SNetIface * pIface,
					 SNetIface ** ppRevIface)
{
  if (!_net_iface_is_bidir(pIface))
    return NET_ERROR_MGMT_IFACE_NOT_SUPPORTED;
  if (!net_iface_is_connected(pIface))
    return NET_ERROR_MGMT_IFACE_NOT_CONNECTED;
  *ppRevIface= pIface->tDest.pIface;
  return NET_SUCCESS;
}

// -----[ net_iface_recv ]-------------------------------------------
int net_iface_recv(SNetIface * pIface, SNetMessage * pMsg)
{
  assert(pIface->fRecv != NULL);
  return pIface->fRecv(pIface, pMsg);
}

// -----[ net_iface_send ]-------------------------------------------
int net_iface_send(SNetIface * pIface, SNetIface ** ppDstIface,
		   net_addr_t tNextHop, SNetMessage ** ppMsg)
{
  assert(pIface->pContext != NULL);
  assert(pIface->fSend != NULL);
  return pIface->fSend(tNextHop, pIface->pContext, ppDstIface, ppMsg);
}


/////////////////////////////////////////////////////////////////////
// ATTRIBUTES
/////////////////////////////////////////////////////////////////////

// -----[ _net_iface_set_flag ]--------------------------------------
static inline void _net_iface_set_flag(SNetIface * pIface,
				       uint8_t uFlag, int iState)
{
  if (iState)
    pIface->uFlags|= uFlag;
  else
    pIface->uFlags&= ~uFlag;
}

// -----[ _net_iface_get_flag ]--------------------------------------
static inline int _net_iface_get_flag(SNetIface * pIface, uint8_t uFlag)
{
  return (pIface->uFlags & uFlag) != 0;
}

// -----[ net_iface_is_enabled ]-------------------------------------
int net_iface_is_enabled(SNetIface * pIface)
{
  return _net_iface_get_flag(pIface, NET_LINK_FLAG_UP);
}

// -----[ net_iface_set_enabled ]------------------------------------
void net_iface_set_enabled(SNetIface * pIface, int iEnabled)
{
  _net_iface_set_flag(pIface, NET_LINK_FLAG_UP, iEnabled);
}

// -----[ net_iface_get_metric ]-------------------------------------
/**
 * To-do: if (tTOS > depth) => return NET_IGP_MAX_WEIGHT
 */
net_igp_weight_t net_iface_get_metric(SNetIface * pIface, net_tos_t tTOS)
{
  if (pIface->pWeights == NULL)
    return NET_IGP_MAX_WEIGHT;
  return pIface->pWeights->data[tTOS];
}

// -----[ net_iface_set_metric ]-------------------------------------
/**
 * To-do: return error if (pWeights == NULL) or if (depth <= tTOS)
 */
int net_iface_set_metric(SNetIface * pIface, net_tos_t tTOS,
			 net_igp_weight_t tWeight, EIfaceDir eDir)
{
  int iResult;
  SNetIface * pRevIface;

  assert(pIface->pWeights != NULL);
  assert(tTOS < net_igp_weights_depth(pIface->pWeights));

  if (eDir == BIDIR) {
    iResult= _net_iface_get_reverse(pIface, &pRevIface);
    if (iResult != NET_SUCCESS)
      return iResult;
    iResult= net_iface_set_metric(pRevIface, tTOS, tWeight, UNIDIR);
    if (iResult != NET_SUCCESS)
      return iResult;
  }

  pIface->pWeights->data[tTOS]= tWeight;
  return NET_SUCCESS;
}

// -----[ net_iface_get_delay ]--------------------------------------
net_link_delay_t net_iface_get_delay(SNetIface * pIface)
{
  return pIface->tDelay;
}

// -----[ net_iface_set_delay ]--------------------------------------
int net_iface_set_delay(SNetIface * pIface, net_link_delay_t tDelay,
			EIfaceDir eDir)
{
  SNetIface * pRevIface;
  int iResult;
  if (eDir == BIDIR) {
    iResult= _net_iface_get_reverse(pIface, &pRevIface);
    if (iResult != NET_SUCCESS)
      return iResult;
    iResult= net_iface_set_delay(pRevIface, tDelay, UNIDIR);
    if (iResult != NET_SUCCESS)
      return iResult;
  }
  pIface->tDelay= tDelay;
  return NET_SUCCESS;
}

// -----[ net_iface_set_capacity ]-----------------------------------
int net_iface_set_capacity(SNetIface * pIface, net_link_load_t tCapacity,
			   EIfaceDir eDir)
{
  SNetIface * pRevIface;
  int iResult;
  if (eDir == BIDIR) {
    iResult= _net_iface_get_reverse(pIface, &pRevIface);
    if (iResult != NET_SUCCESS)
      return iResult;
    iResult= net_iface_set_capacity(pRevIface, tCapacity, UNIDIR);
    if (iResult != NET_SUCCESS)
      return iResult;
  }
  pIface->tCapacity= tCapacity;
  return NET_SUCCESS;
}

// -----[ net_iface_get_capacity ]-----------------------------------
net_link_load_t net_iface_get_capacity(SNetIface * pIface)
{
  return pIface->tCapacity;
}

// -----[ net_iface_set_load ]---------------------------------------
void net_iface_set_load(SNetIface * pIface, net_link_load_t tLoad)
{
  pIface->tLoad= tLoad;
}

// -----[ net_iface_get_load ]---------------------------------------
net_link_load_t net_iface_get_load(SNetIface * pIface)
{
  return pIface->tLoad;
}

// -----[ net_iface_add_load ]---------------------------------------
void net_iface_add_load(SNetIface * pIface, net_link_load_t tIncLoad)
{
  uint64_t tBigLoad= pIface->tLoad;
  tBigLoad+= tIncLoad;
  if (tBigLoad > NET_LINK_MAX_LOAD)
    pIface->tLoad= NET_LINK_MAX_LOAD;
  else
    pIface->tLoad= (net_link_load_t) tBigLoad;
}


/////////////////////////////////////////////////////////////////////
// DUMP
/////////////////////////////////////////////////////////////////////

// -----[ net_iface_dump_type ]--------------------------------------
void net_iface_dump_type(SLogStream * pStream, SNetIface * pIface)
{
  char * pcName= net_iface_type2name(pIface->tType, 1);
  assert(pcName != NULL);
  log_printf(pStream, "%s", pcName);
}

// -----[ net_iface_dump_id ]----------------------------------------
void net_iface_dump_id(SLogStream * pStream, SNetIface * pIface)
{
  ip_prefix_dump(pStream, net_iface_id(pIface));
}

// -----[ net_iface_dump_dest ]--------------------------------------
void net_iface_dump_dest(SLogStream * pStream, SNetIface * pIface)
{
  if (!pIface->uConnected) {
    log_printf(pStream, "not-connected");
    return;
  }
  switch (pIface->tType) {
  case NET_IFACE_RTR:
    ip_address_dump(pStream, pIface->tIfaceAddr);
    break;
  case NET_IFACE_PTP:
    net_iface_dump_id(pStream, pIface->tDest.pIface);
    log_printf(pStream, "\t");
    ip_address_dump(pStream, pIface->tDest.pIface->pSrcNode->tAddr);
    break;
  case NET_IFACE_PTMP:
    ip_prefix_dump(pStream, pIface->tDest.pSubnet->sPrefix);
    break;
  case NET_IFACE_VIRTUAL:
    ip_address_dump(pStream, pIface->tDest.tEndPoint);
  default:
    abort();
  }
}

// -----[ net_iface_dump ]-------------------------------------------
void net_iface_dump(SLogStream * pStream, SNetIface * pIface, int withDest)
{
  net_iface_dump_type(pStream, pIface);
  log_printf(pStream, "\t");
  net_iface_dump_id(pStream, pIface);
  log_printf(pStream, "\t");
  if (withDest)
    net_iface_dump_dest(pStream, pIface);
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
static int _net_iface_dumpf(SLogStream * pStream, void * pContext,
			    char cFormat)
{
  SNetIface * pIface= (SNetIface *) pContext;
  switch (cFormat) {
  case 'D':
    net_iface_dump_dest(pStream, pIface);
    break;
  case 'I':
    net_iface_dump_id(pStream, pIface);
    break;
  case 'c':
    log_printf(pStream, "%u", pIface->tCapacity);
    break;
  case 'd':
    log_printf(pStream, "%u", pIface->tDelay);
    break;
  case 'l':
    log_printf(pStream, "%u", pIface->tLoad);
    break;
  case 's':
    if (_net_iface_get_flag(pIface, NET_LINK_FLAG_UP))
      log_printf(pStream, "UP");
    else
      log_printf(pStream, "DOWN");
    break;
  case 'w':
    if (pIface->pWeights != NULL)
      log_printf(pStream, "%u", pIface->pWeights->data[0]);
    else
      log_printf(pStream, "undef");
    break;
  default:
    return -1;
  }
  return 0;
}

// -----[ net_iface_dumpf ]------------------------------------------
void net_iface_dumpf(SLogStream * pStream, SNetIface * pIface,
		     const char * pcFormat)
{
  int iResult;
  iResult= str_format_for_each(pStream, _net_iface_dumpf, pIface, pcFormat);
  assert(iResult == 0);
}



/////////////////////////////////////////////////////////////////////
// STRING TO TYPE / IDENTIFIER CONVERSION
/////////////////////////////////////////////////////////////////////

// -----[ net_iface_str2type ]---------------------------------------
net_iface_type_t net_iface_str2type(char * pcType)
{
  unsigned int uIndex= 0;
  while (uIndex < IFACE_NUM_TYPES) {
    if (!strcmp(pcType, IFACE_TYPES[uIndex].pcName) ||
	!strcmp(pcType, IFACE_TYPES[uIndex].pcShortName))
      return IFACE_TYPES[uIndex].tType;
    uIndex++;
  }
  return NET_IFACE_UNKNOWN;
}

// -----[ net_iface_str2id ]-----------------------------------------
/**
 * Convert a string to an interface identifier (ID). The string can
 * be an IP address or a prefix.
 */
int net_iface_str2id(char * pcID, net_iface_id_t * ptID)
{
  SNetDest sID;

  // Destination must be an IP address or an IP prefix
  if ((ip_string_to_dest(pcID, &sID) < 0) ||
      (sID.tType == NET_DEST_ANY) ||
      (sID.tType == NET_DEST_INVALID)) {
    return NET_ERROR_MGMT_INVALID_IFACE_ID;
  }

  // Convert destination to prefix (interface ID)
  *ptID= net_dest_to_prefix(sID);
  return NET_SUCCESS;
}
