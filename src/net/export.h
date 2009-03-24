// ==================================================================
// @(#)export.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export.h,v 1.2 2009-03-24 16:07:06 bqu Exp $
// ==================================================================

#ifndef __NET_EXPORT_H__
#define __NET_EXPORT_H__

#include <libgds/stream.h>
#include <net/net_types.h>

typedef enum {
  NET_EXPORT_FORMAT_CLI,
  NET_EXPORT_FORMAT_DOT,
  NET_EXPORT_FORMAT_NTF,
  NET_EXPORT_FORMAT_MAX
} net_export_format_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_stream ]--------------------------------------
  int net_export_stream(gds_stream_t * stream,
			network_t * network,
			net_export_format_t format);
  // -----[ net_export_file ]----------------------------------------
  int net_export_file(const char * filename,
		      network_t * network,
		      net_export_format_t format);

  // -----[ net_export_format2str ]----------------------------------
  const char * net_export_format2str(net_export_format_t format);
  // -----[ net_export_str2format ]----------------------------------
  int net_export_str2format(const char * str,
			    net_export_format_t * format);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_H__ */
