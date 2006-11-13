// ==================================================================
// @(#)protocol.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 27/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/memory.h>
#include <string.h>

#include <net/protocol.h>

// Protocol names
const char * PROTOCOL_NAMES[]= {
  "icmp",
  "bgp",
  "ipip"
};

// ----- protocol_create --------------------------------------------
/**
 *
 */
SNetProtocol * protocol_create(void * pHandler,
			       FNetNodeHandlerDestroy fDestroy,
			       FNetNodeHandleEvent fHandleEvent)
{
  SNetProtocol * pProtocol=
    (SNetProtocol *) MALLOC(sizeof(SNetProtocol));
  pProtocol->pHandler= pHandler;
  pProtocol->fDestroy= fDestroy;
  pProtocol->fHandleEvent= fHandleEvent;
  return pProtocol;
}

// ----- protocol_destroy -------------------------------------------
/**
 *
 */
void protocol_destroy(SNetProtocol ** ppProtocol)
{
  if (*ppProtocol != NULL) {
    if ((*ppProtocol)->fDestroy != NULL)
      (*ppProtocol)->fDestroy(&(*ppProtocol)->pHandler);
    FREE(*ppProtocol);
    *ppProtocol= NULL;
  }
}

// ----- protocols_create -------------------------------------------
/**
 *
 */
SNetProtocols * protocols_create()
{
  SNetProtocols * pProtocols=
    (SNetProtocols *) MALLOC(sizeof(SNetProtocols));

  memset(pProtocols->data, 0, sizeof(pProtocols->data));
  return pProtocols;
}

// ----- protocols_destroy ------------------------------------------
/**
 * This function destroys the list of protocols and destroys all the
 * related protocol instances if required.
 */
void protocols_destroy(SNetProtocols ** ppProtocols)
{
  int iIndex;

  if (*ppProtocols != NULL) {
    
    // Destroy protocols (also clear the protocol instance if
    // required, see 'protocol_destroy')
    for (iIndex= 0; iIndex < NET_PROTOCOL_MAX; iIndex++)
      if ((*ppProtocols)->data[iIndex] != NULL)
	protocol_destroy(&(*ppProtocols)->data[iIndex]);

    FREE(*ppProtocols);
    *ppProtocols= NULL;
  }
}

// ----- protocols_register -----------------------------------------
/**
 *
 */
int protocols_register(SNetProtocols * pProtocols,
		       uint8_t uNumber, void * pHandler,
		       FNetNodeHandlerDestroy fDestroy,
		       FNetNodeHandleEvent fHandleEvent)
{
  if (uNumber >= NET_PROTOCOL_MAX)
    return -1;

  if (pProtocols->data[uNumber] != NULL)
    return -1;

  pProtocols->data[uNumber]= protocol_create(pHandler, fDestroy,
					     fHandleEvent);

  return 0;
}

// ----- protocols_get ----------------------------------------------
/**
 *
 */
SNetProtocol * protocols_get(SNetProtocols * pProtocols,
			     uint8_t uNumber)
{
  if (uNumber >= NET_PROTOCOL_MAX)
    return NULL;
  
  return pProtocols->data[uNumber];
}
