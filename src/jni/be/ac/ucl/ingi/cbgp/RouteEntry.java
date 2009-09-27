// ==================================================================
// @(#)RouteEntry.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/09/2008
// $Id$
// ==================================================================

package be.ac.ucl.ingi.cbgp;

public class RouteEntry {

	public final IPAddress oif;
	public final IPAddress gateway;
	
	// -----[ constructor ]-----------------------------------------
	public RouteEntry(IPAddress oif, IPAddress gateway) {
		this.oif= oif;
		this.gateway= gateway;
    }

}
