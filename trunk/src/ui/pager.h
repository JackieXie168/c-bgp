// ==================================================================
// @(#)pager.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/11/2007
// @lastdate 04/12/2007
// ==================================================================

#ifndef __UI_PAGER_H__
#define __UI_PAGER_H__

#define PAGER_CMD "less"

#define PAGER_SUCCESS 0

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ pager_run ]----------------------------------------------
  int pager_run(char * pcFileName);

#ifdef __cplusplus
}
#endif

#endif /* __UI_PAGER_H__ */
