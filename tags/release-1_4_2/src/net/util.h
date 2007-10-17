// ==================================================================
// @(#)util.h
//
// Various network utility function: conversion from string to
// address and so on...
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/01/2007
// @lastdate 23/01/2007
// ==================================================================

#ifndef __NET_UTIL_H__
#define __NET_UTIL_H__

#include <net/prefix.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern"C" {
#endif

  // ----- str2address ----------------------------------------------
  int str2address(char * pcValue, net_addr_t * pAddr);
  // ----- str2prefix -------------------------------------------------
  int str2prefix(char * pcValue, SPrefix * pPrefix);
  // ----- str2tos ----------------------------------------------------
  int str2tos(char * pcValue, net_tos_t * pTOS);

#ifdef __cplusplus
}
#endif

#endif /* __NET_UTIL_H__ */
