// ==================================================================
// @(#)net_path.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 09/07/2003
// $Id: net_path.c,v 1.9 2009-03-24 16:18:21 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <net/net_path.h>
#include <libgds/memory.h>
#include <string.h>

// ----- net_path_create --------------------------------------------
net_path_t * net_path_create()
{
  return uint32_array_create(0);
}

// ----- net_path_destroy -------------------------------------------
void net_path_destroy(net_path_t ** path_ref)
{
  uint32_array_destroy(path_ref);
}

// ----- net_path_append --------------------------------------------
/**
 *
 */
int net_path_append(net_path_t * path, net_addr_t addr)
{
  return uint32_array_append(path, addr);
}

// ----- net_path_copy ----------------------------------------------
/**
 *
 */
net_path_t * net_path_copy(net_path_t * path)
{
  return uint32_array_copy(path);
}

// ----- net_path_length --------------------------------------------
/**
 *
 */
int net_path_length(net_path_t * path)
{
  return uint32_array_size(path);
}

// ----- net_path_for_each ------------------------------------------
/**
 * Call the given function with the given context for each element of
 * the given path. Each element is an IP address (i.e. has the
 * 'net_addr_t' type).
 */
int net_path_for_each(net_path_t * path, gds_array_foreach_f foreach,
		      void * ctx)
{
  return uint32_array_for_each(path, foreach, ctx);
}

// ----- net_path_dump ----------------------------------------------
/**
 *
 */
void net_path_dump(gds_stream_t * stream, net_path_t * path)
{
  unsigned int index;

  for (index= 0; index < uint32_array_size(path); index++) {
    if (index > 0)
      stream_printf(stream, " ");
    ip_address_dump(stream, path->data[index]);
  }
}

// ----- net_path_search ---------------------------------------------
/**
 *
 */
int net_path_search(net_path_t * path, net_addr_t addr)
{
  unsigned int index;
  int iRet = 0;

  for (index= 0; index < uint32_array_size(path); index++) {
    if (addr == path->data[index]) {
      iRet = 1;
      break;
    }
  }

  return iRet;
}
