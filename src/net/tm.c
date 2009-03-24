// ==================================================================
// @(#)tm.c
//
// Traffic matrix management.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/01/2007
// $Id: tm.c,v 1.6 2009-03-24 16:28:11 bqu Exp $
// ==================================================================
// TODO:
//   - what action must be taken in case of incomplete record-route
//     (UNREACH, ...)
//   - inbound interface (in-if) is not yet supported (field is
//     ignored)
//   - TOS field is not yet supported (field is ignored)
//   - there is some redundancy with the NetFlow handling regarding
//     how record-route is configured and performed.
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <libgds/str_util.h>
#include <libgds/tokenizer.h>
#include <net/network.h>
#include <net/node.h>
#include <net/icmp.h>
#include <net/icmp_options.h>
#include <net/tm.h>
#include <net/util.h>
#include <util/lrp.h>

#define MAX_TM_LINE_LEN 80

static lrp_t * _parser= NULL;

#define DEBUG
#include <libgds/debug.h>

// -----[ net_tm_perror ]--------------------------------------------
void net_tm_perror(gds_stream_t * stream, int error)
{
  char * error_str= net_tm_strerror(error);
  if (error_str != NULL)
    stream_printf(stream, error_str);
  else
    stream_printf(stream, "unknown error (%d)", error);
}

// -----[ net_tm_strerror ]--------------------------------------------
char * net_tm_strerror(int error)
{
  switch (error) {
  case NET_TM_SUCCESS:
    return "success";
  case NET_TM_ERROR:
    return "undefined error";
  case NET_TM_ERROR_OPEN:
    return "failed to open file";
  case NET_TM_ERROR_NUM_PARAMS:
    return "not enough fields";
  case NET_TM_ERROR_INVALID_SRC:
    return "invalid source";
  case NET_TM_ERROR_UNKNOWN_SRC:
    return "source not found";
  case NET_TM_ERROR_INVALID_DST:
    return "invalid destination";
  case NET_TM_ERROR_INVALID_LOAD:
    return "invalid load";
  }
  return NULL;
}

// -----[ _parse ]---------------------------------------------------
static inline int _parse(lrp_t * parser)
{
  unsigned int num_fields;
  const char * field;
  const char * field_src, * field_dst;
  int result;
  net_addr_t src_addr;
  net_node_t * node;
  ip_dest_t dest;
  net_link_load_t load;
  network_t * network= network_get_default();

  while (lrp_get_next_line(parser)) {
    if (lrp_get_num_fields(parser, &num_fields) < 0)
      return -1;

    // We should have at least 4 tokens
    if (num_fields != 4) {
      lrp_set_user_error(parser, "incorrect number of fields (4 expected)");
      return NET_TM_ERROR_NUM_PARAMS;
    }

    // Source node
    field_src= lrp_get_field(parser, 0);
    if (str2address(field_src, &src_addr) < 0) {
      lrp_set_user_error(parser, "invalid source \"%s\"", field_src);
      return NET_TM_ERROR_INVALID_SRC;
    }
    node= network_find_node(network, src_addr);
    if (node == NULL) {
      lrp_set_user_error(parser, "unknown source \"%s\"", field_src);
      return NET_TM_ERROR_UNKNOWN_SRC;
    }
    
    // Source interface
    field= lrp_get_field(parser, 1);
    // Not used for forwarding (yet)

    // Destination prefix
    field_dst= lrp_get_field(parser, 2);
    if (ip_string_to_dest(field_dst, &dest) < 0) {
      lrp_set_user_error(parser, "invalid destination \"%s\"", field_dst);
      return NET_TM_ERROR_INVALID_DST;
    }

    // Load (volume of traffic)
    field= lrp_get_field(parser, 3);
    if (str2capacity(field, &load) < 0) {
      lrp_set_user_error(parser, "invalid load \"%s\"", field);
      return NET_TM_ERROR_INVALID_LOAD;
    }

    // Optional TOS ?
    // --> Not supported (yet)

    __debug("load traffic\n"
	    "  +-- from  :%s\n"
	    "  +-- to    :%s\n",
	    "  +-- volume:%u\n", 
	    field_src, field_dst, (unsigned int) load);

    result= node_load_flow(node, IP_ADDR_ANY, dest.addr, load, NULL, NULL);
    if (result < 0)
      return NET_TM_ERROR;
  }
  return NET_TM_SUCCESS;
}

// -----[ net_tm_parser ]--------------------------------------------
int net_tm_parser(FILE * stream)
{
  lrp_set_stream(_parser, stream);
  return _parse(_parser);
}

// -----[ net_tm_load ]----------------------------------------------
int net_tm_load(const char * filename)
{
  int result= lrp_open(_parser, filename);
  if (result == 0)
    result= _parse(_parser);
  lrp_close(_parser);
  return result;
}

// -----[ _tm_init ]-------------------------------------------------
void _tm_init()
{
  _parser= lrp_create(MAX_TM_LINE_LEN, " \t");
}

// -----[ _tm_done ]-------------------------------------------------
void _tm_done()
{
  if (_parser != NULL)
    lrp_destroy(&_parser);
}
