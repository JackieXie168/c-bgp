// ==================================================================
// @(#)Route.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/02/2005
// $Id: Route.java,v 1.3 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp; 


// -----[ Route ]----------------------------------------------------
/**
 * This is a basic implementation of the RouteInterface.
 */
public class Route {

    // -----[ protected attributes of the route ]-------------------
    protected final IPPrefix prefix;
    protected final RouteEntry [] entries;
    
    // -----[ Route ]-----------------------------------------------
    public Route(IPPrefix prefix, RouteEntry [] entries) {
    	this.prefix= prefix;
    	this.entries= entries;
    }

    // -----[ getOutIface ]-----------------------------------------
    public IPAddress getOutIface() {
    	return entries[0].oif;
    }
    
    // -----[ getGateway ]------------------------------------------
    public IPAddress getGateway() {
    	return entries[0].gateway;
    }

    // -----[ getPrefix ]-------------------------------------------
    public IPPrefix getPrefix() {
    	return prefix;
    }
    
    // -----[ getEntries ]------------------------------------------
    public RouteEntry [] getEntries() {
    	return entries;
    }
    
    // -----[ getEntriesCount ]-------------------------------------
    public int getEntriesCount() {
    	return entries.length;
    }
    
}
