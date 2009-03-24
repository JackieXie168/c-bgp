// ==================================================================
// @(#)str_format.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/02/2008
// $Id: str_format.c,v 1.2 2009-03-24 16:30:03 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <util/str_format.h>

typedef enum {
  NORMAL, ESCAPED
} EFormatState;

// -----[ str_format_for_each ]--------------------------------------
int str_format_for_each(gds_stream_t * stream,
			FFormatForEach fFunction,
			void * pContext,
			const char * pcFormat)
{
  const char * pPos= pcFormat;
  EFormatState eState= NORMAL;
  int iResult;

  while ((pPos != NULL) && (*pPos != 0)) {
    switch (eState) {
    case NORMAL:
      if (*pPos == '%') {
	eState= ESCAPED;
      } else {
	stream_printf(stream, "%c", *pPos);
      }
      break;

    case ESCAPED:
      if (*pPos != '%') {
	iResult= fFunction(stream, pContext, *pPos);
	if (iResult != 0)
	  return iResult;
      } else
	stream_printf(stream, "%");
      eState= NORMAL;
      break;

    default:
      abort();
    }
    pPos++;
  }
  // only NORMAL is a final state for the FSM
  if (eState != NORMAL)
    return -1;
  return 0;
}

