// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 03/04/08
// $Id: main-test.c,v 1.15 2008-06-16 09:58:24 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <api.h>
#include <selfcheck.h>

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  int result;

  libcbgp_init();
  libcbgp_banner();
  result= libcbgp_selfcheck();
  libcbgp_done();

  return (result==0?EXIT_SUCCESS:EXIT_FAILURE);
}
