// ==================================================================
// @(#)pager.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/11/2007
// $Id: pager.h,v 1.2 2009-03-24 16:29:41 bqu Exp $
// ==================================================================

#ifndef __UI_PAGER_H__
#define __UI_PAGER_H__

#define PAGER_CMD "less"

#define PAGER_SUCCESS 0

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ pager_run ]----------------------------------------------
  int pager_run(const char * filename);

#ifdef __cplusplus
}
#endif

#endif /* __UI_PAGER_H__ */
