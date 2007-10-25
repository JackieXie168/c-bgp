// ==================================================================
// @(#)util.c
//
// Various network utility function: conversion from string to
// address and so on...
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/01/2007
// @lastdate 23/01/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/str_util.h>

#include <net/prefix.h>
#include <net/util.h>

// ----- str2address ------------------------------------------------
int str2address(char * pcValue, net_addr_t * pAddr)
{
  char * pcEndPtr;

  if (ip_string_to_address(pcValue, &pcEndPtr, pAddr) ||
      (*pcEndPtr != 0))
    return -1;
  return 0;
}

// ----- str2prefix -------------------------------------------------
int str2prefix(char * pcValue, SPrefix * pPrefix)
{
  char * pcEndPtr;

  if (ip_string_to_prefix(pcValue, &pcEndPtr, pPrefix) ||
      (*pcEndPtr != 0))
    return -1;
  return 0;
}

// ----- str2tos ----------------------------------------------------
int str2tos(char * pcValue, net_tos_t * pTOS)
{
  unsigned int uTOS;

  if (str_as_uint(pcValue, &uTOS) < 0)
    return -1;

  *pTOS= (net_tos_t) uTOS;
  return 0;
}
