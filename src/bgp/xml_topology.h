// ==================================================================
// @(#)xml_topology.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/09/2003
// @lastdate 24/11/2004
// ==================================================================

#ifndef __BGP_XML_H__
#define __BGP_XML_H__

#include <config.h>
#include <net/network.h>

#ifdef HAVE_XML

// ----- xml_topology_load -------------------------------------------
int xml_topology_load(char * cFile, SNetwork * pNetwork);
// ----- xml_topology_dump -------------------------------------------
int xml_topology_dump(SNetwork * pNetwork, char * pcFile);

#endif

#endif
