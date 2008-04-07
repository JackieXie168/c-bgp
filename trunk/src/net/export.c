// ==================================================================
// @(#)export.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/10/07
// @lastdate 15/10/07
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <net/error.h>
#include <net/export.h>
#include <net/export_cli.h>
#include <net/export_ntf.h>
#include <net/network.h>

// -----[ net_export_stream ]----------------------------------------
/**
 *
 */
int net_export_stream(SLogStream * pStream)
{
  return net_export_cli(pStream, network_get_default());
}

// -----[ net_export_file ]------------------------------------------
/**
 *
 */
int net_export_file(const char * pcFileName)
{
  SLogStream * pStream= log_create_file((char *) pcFileName);
  int iResult;

  if (pStream == NULL)
    return -1;

  // Content
  iResult= net_export_stream(pStream);

  log_destroy(&pStream);

  return iResult;
}
