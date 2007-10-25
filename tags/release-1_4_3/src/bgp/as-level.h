// ==================================================================
// @(#)as-level.h
//
// Provide structures and functions to handle an AS-level topology
// with business relationships. This is the foundation for handling
// AS-level topologies inferred by Subramanian et al and further by
// CAIDA.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/04/2007
// @lastdate 16/10/2007
// ==================================================================

#ifndef __BGP_ASLEVEL_H__
#define __BGP_ASLEVEL_H__

#include <bgp/filter.h>
#include <net/prefix.h>

// ----- Addressing schemes -----
#define ASLEVEL_ADDR_SCH_GLOBAL  0
#define ASLEVEL_ADDR_SCH_LOCAL   1
#define ASLEVEL_ADDR_SCH_DEFAULT ASLEVEL_ADDR_SCH_GLOBAL

// ----- File format -----
#define ASLEVEL_FORMAT_REXFORD 0
#define ASLEVEL_FORMAT_CAIDA   1
#define ASLEVEL_FORMAT_MEULLE  2
#define ASLEVEL_FORMAT_DEFAULT ASLEVEL_FORMAT_REXFORD

// ----- Topology state -----
#define ASLEVEL_STATE_LOADED    0
#define ASLEVEL_STATE_INSTALLED 1
#define ASLEVEL_STATE_POLICIES  2
#define ASLEVEL_STATE_RUNNING   3

// ----- Local preferences -----
#define ASLEVEL_PREF_PROV 60
#define ASLEVEL_PREF_PEER 80
#define ASLEVEL_PREF_CUST 100

// ----- Error codes -----
#define ASLEVEL_SUCCESS                 0
#define ASLEVEL_ERROR_UNEXPECTED        -1
#define ASLEVEL_ERROR_OPEN              -2
#define ASLEVEL_ERROR_NUM_PARAMS        -3
#define ASLEVEL_ERROR_INVALID_ASNUM     -4
#define ASLEVEL_ERROR_INVALID_RELATION  -5
#define ASLEVEL_ERROR_INVALID_DELAY     -6
#define ASLEVEL_ERROR_DUPLICATE_LINK    -7
#define ASLEVEL_ERROR_LOOP_LINK         -8
#define ASLEVEL_ERROR_NODE_EXISTS       -9
#define ASLEVEL_ERROR_NO_TOPOLOGY       -10
#define ASLEVEL_ERROR_TOPOLOGY_LOADED   -11
#define ASLEVEL_ERROR_UNKNOWN_FORMAT    -12
#define ASLEVEL_ERROR_UNKNOWN_ADDRSCH   -13
#define ASLEVEL_ERROR_CYCLE_DETECTED    -14
#define ASLEVEL_ERROR_DISCONNECTED      -15
#define ASLEVEL_ERROR_INCONSISTENT      -16
#define ASLEVEL_ERROR_UNKNOWN_FILTER    -17
#define ASLEVEL_ERROR_INVALID_STATE     -18
#define ASLEVEL_ERROR_NOT_INSTALLED     -19
#define ASLEVEL_ERROR_ALREADY_INSTALLED -20
#define ASLEVEL_ERROR_ALREADY_RUNNING   -21

// ----- Business relationships -----
#define ASLEVEL_PEER_TYPE_CUSTOMER 0
#define ASLEVEL_PEER_TYPE_PROVIDER 1
#define ASLEVEL_PEER_TYPE_PEER     2
#define ASLEVEL_PEER_TYPE_SIBLING  3
#define ASLEVEL_PEER_TYPE_UNKNOWN  255

typedef uint8_t peer_type_t;

// ----- Addressing scheme function -----
/** maps an ASN to a router identifier (IPv4 address) */
typedef net_addr_t (*FASLevelAddrMapper)(uint16_t uASN);

// ----- AS-level topology structure -----
typedef struct TASLevelTopo SASLevelTopo;
typedef struct TASLevelDomain SASLevelDomain;
typedef struct TASLevelLink SASLevelLink;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_perror ]-----------------------------------------
  void aslevel_perror(SLogStream * pStream, int iErrorCode);


  ///////////////////////////////////////////////////////////////////
  // TOPOLOGY MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_topo_create ]------------------------------------
  SASLevelTopo * aslevel_topo_create(uint8_t uAddrScheme);
  // -----[ aslevel_topo_destroy ]-----------------------------------
  void aslevel_topo_destroy(SASLevelTopo ** ppTopo);
  // -----[ aslevel_topo_add_as ]------------------------------------
  SASLevelDomain * aslevel_topo_add_as(SASLevelTopo * pTopo,
				       uint16_t uASN);
  // -----[ aslevel_top_get_as ]-------------------------------------
  SASLevelDomain *  aslevel_topo_get_as(SASLevelTopo * pTopo,
					uint16_t uASN);
  // -----[ aslevel_as_num_providers ]-------------------------------
  unsigned int aslevel_as_num_providers(SASLevelDomain * pDomain);
  // -----[ aslevel_as_add_link ]------------------------------------
  int aslevel_as_add_link(SASLevelDomain * pDomain1,
			  SASLevelDomain * pDomain2,
			  peer_type_t tPeerType,
			  SASLevelLink ** ppLink);
  // -----[ aslevel_as_get_link ]------------------------------------
  SASLevelLink * aslevel_as_get_link(SASLevelDomain * pDomain1,
				      SASLevelDomain * pDomain2);
  // -----[ aslevel_link_get_peer_type ]-----------------------------
  peer_type_t aslevel_link_get_peer_type(SASLevelLink * pLink);

  // -----[ aslevel_topo_check_cycle ]-------------------------------
  int aslevel_topo_check_cycle(SASLevelTopo * pTopo, int iVerbose);
  // -----[ aslevel_topo_check_consistency ]-------------------------
  int aslevel_topo_check_consistency(SASLevelTopo * pTopo);

  // -----[ aslevel_topo_build_network ]-----------------------------
  int aslevel_topo_build_network(SASLevelTopo * pTopo);

  // -----[ aslevel_topo_info ]--------------------------------------
  int aslevel_topo_info(SLogStream * pStream);
  // -----[ aslevel_topo_num_nodes ]---------------------------------
  unsigned int aslevel_topo_num_nodes(SASLevelTopo * pTopo);
  // -----[ aslevel_as_num_customers ]-------------------------------
  unsigned int aslevel_as_num_customers(SASLevelDomain * pDomain);
  // -----[ aslevel_as_num_providers ]-------------------------------
  unsigned int aslevel_as_num_providers(SASLevelDomain * pDomain);


  ///////////////////////////////////////////////////////////////////
  // TOPOLOGY API
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_topo_str2format ]--------------------------------
  int aslevel_topo_str2format(const char * pcFormat, uint8_t * puFormat);

  // -----[ aslevel_topo_load ]--------------------------------------
  int aslevel_topo_load(const char * pcFileName, uint8_t uFormat,
			uint8_t uAddrScheme);
  // -----[ aslevel_topo_check ]-------------------------------------
  int aslevel_topo_check(int iVerbose);
  // -----[ aslevel_topo_filter ]------------------------------------
  int aslevel_topo_filter(uint8_t uFilter);
  // -----[ aslevel_topo_install ]-----------------------------------
  int aslevel_topo_install();
  // -----[ aslevel_topo_policies ]----------------------------------
  int aslevel_topo_policies();
  // -----[ aslevel_topo_run ]---------------------------------------
  int aslevel_topo_run();

  // -----[ aslevel_get_topo ]---------------------------------------
  SASLevelTopo * aslevel_get_topo();

  // -----[ aslevel_topo_dump_stubs ]--------------------------------
  int aslevel_topo_dump_stubs(SLogStream * pStream);
  // -----[ aslevel_topo_record_route ]--------------------------------
  int aslevel_topo_record_route(SLogStream * pStream, SPrefix sPrefix);

  ///////////////////////////////////////////////////////////////////
  // FILTERS MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_str2peer_type ]----------------------------------
  peer_type_t aslevel_str2peer_type(const char * pcStr);
  // -----[ aslevel_reverse_relation ]-------------------------------
  peer_type_t aslevel_reverse_relation(peer_type_t tPeerType);
  // -----[ aslevel_filter_in ]--------------------------------------
  SFilter * aslevel_filter_in(peer_type_t tPeerType);
  // -----[ aslevel_filter_out ]-------------------------------------
  SFilter * aslevel_filter_out(peer_type_t tPeerType);


  ///////////////////////////////////////////////////////////////////
  // ADDRESSING SCHEMES MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_str2addr_sch ]-----------------------------------
  int aslevel_str2addr_sch(const char * pcStr, uint8_t * puAddrScheme);
  // -----[ aslevel_addr_sch_default_get ]---------------------------
  net_addr_t aslevel_addr_sch_default_get(uint16_t uASN);
  // -----[ aslevel_addr_sch_local_get ]-----------------------------
  net_addr_t aslevel_addr_sch_local_get(uint16_t uASN);


  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION & FINALIZATION
  ///////////////////////////////////////////////////////////////////

  // -----[ _aslevel_destroy ]---------------------------------------
  void _aslevel_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_H__ */
