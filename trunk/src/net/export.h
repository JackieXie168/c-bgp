// ==================================================================
// @(#)export.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/10/07
// @lastdate 15/10/07
// ==================================================================

#ifndef __NET_EXPORT_H__
#define __NET_EXPORT_H__

#include <libgds/log.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_stream ]--------------------------------------
  int net_export_stream(SLogStream * pStream);
  // -----[ net_export_file ]----------------------------------------
  int net_export_file(const char * pcFileName);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_H__ */
