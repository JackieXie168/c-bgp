// ==================================================================
// @(#)xml_topology.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/09/2003
// $Id: xml_topology.h,v 1.4 2009-03-24 15:56:22 bqu Exp $
// ==================================================================

#ifndef __BGP_XML_H__
#define __BGP_XML_H__

#include <net/network.h>

// ----- xml_topology_load -------------------------------------------
int xml_topology_load(char * cFile, SNetwork * pNetwork);
// ----- xml_topology_dump -------------------------------------------
int xml_topology_dump(SNetwork * pNetwork, char * pcFile);

#endif
