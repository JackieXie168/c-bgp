// ==================================================================
// @(#)util.h
//
// Various network utility function: conversion from string to
// address and so on...
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/01/2007
// $Id: util.h,v 1.2 2009-03-24 16:28:39 bqu Exp $
// ==================================================================

#ifndef __NET_UTIL_H__
#define __NET_UTIL_H__

#include <net/prefix.h>
#include <net/net_types.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern"C" {
#endif

  // ----- str2address ----------------------------------------------
  int str2address(const char * str, net_addr_t * addr);
  // ----- str2prefix -----------------------------------------------
  int str2prefix(const char * str, ip_pfx_t * prefix);
  // ----- str2tos --------------------------------------------------
  int str2tos(const char * str, net_tos_t * tos);
  // -----[ str2depth ]----------------------------------------------
  int str2depth(const char * str, uint8_t * depth);
  // -----[ str2capacity ]-------------------------------------------
  int str2capacity(const char * str, net_link_load_t * capacity);
  // -----[ str2delay ]----------------------------------------------
  int str2delay(const char * str, net_link_delay_t * delay);
  // -----[ str2weight ]---------------------------------------------
  int str2weight(const char * str, igp_weight_t * weight);
  // -----[ str2domain_id ]------------------------------------------
  int str2domain_id(const char * str, unsigned int * id);
  // -----[ str2ttl ]------------------------------------------------
  int str2ttl(const char * str, uint8_t * ttl);
  // -----[ str2asn ]------------------------------------------------
  int str2asn(const char * str, asn_t * asn);

#ifdef __cplusplus
}
#endif

#endif /* __NET_UTIL_H__ */
