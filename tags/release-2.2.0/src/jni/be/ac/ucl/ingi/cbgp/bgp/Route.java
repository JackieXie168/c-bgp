// ==================================================================
// @(#)Route.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// $Id: Route.java,v 1.4 2009-08-31 09:42:43 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Properties;

import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPPrefix;

// -----[ BGPRoute ]-------------------------------------------------
/**
 * This class is a container for BGP route information.
 */
public class Route {

    // -----[ public constants ]-------------------------------------
    public static final byte ORIGIN_IGP= 0;
    public static final byte ORIGIN_EGP= 1;
    public static final byte ORIGIN_INCOMPLETE= 2;

    // -----[ private attributes of the route ]----------------------
    protected final IPPrefix prefix;
    protected final boolean best;
    protected final boolean feasible;
    protected final IPAddress nexthop;
    protected boolean internal;
    protected long localPref;
    protected long med;
    protected byte origin;
    protected ASPath path;
    protected Communities communities;
    
    protected final Properties properties= new Properties();

    // -----[ BGPRoute ]---------------------------------------------
    /**
     * BGPRoute's constructor. Builds a BGP route with its
     * attributes. Currently only supports destination prefix,
     * next-hop, AS-Path, origin, local-pref, MED and communities.
     */
    public Route(IPPrefix prefix, IPAddress nexthop,
    		long localPref, long med,
		    boolean best, boolean feasible, byte origin,
		    ASPath path, boolean internal,
		    Communities communities) {
    	this.prefix= prefix;
    	this.nexthop= nexthop;
    	this.best= best;
    	this.feasible= feasible;
    	this.localPref= localPref;
    	this.med= med;
    	this.origin= origin;
    	this.path= path;
    	this.internal= internal;
    	this.communities= communities;
    }
    
    // -----[ getPrefix ]--------------------------------------------
    public IPPrefix getPrefix() {
    	return prefix;
    }
    
    // -----[ getNextHop ]-------------------------------------------
    public IPAddress getNextHop() {
    	return nexthop;
    }
    
    // -----[ isBest ]----------------------------------------------
    public boolean isBest() {
    	return best;
    }

    // -----[ isFeasible ]------------------------------------------
    public boolean isFeasible() {
    	return feasible;
    }

    // -----[ getLocalPref ]-----------------------------------------
    /**
     * Returns the route's local-preference.
     */
    public long getLocalPref() {
    	return localPref;
    }

    // -----[ getMED ]-----------------------------------------------
    /**
     * Returns the routes's MED.
     */
    public long getMED() {
    	return med;
    }

    // -----[ getOrigin ]--------------------------------------------
    /**
     * Returns the route's origin.
     */
    public byte getOrigin() {
    	return origin;
    }

    // -----[ getPath ]----------------------------------------------
    /**
     * Returns the route's AS-Path.
     */
    public ASPath getPath() {
    	return path;
    }

    // -----[ getCommunities ]---------------------------------------
    /**
     * Returns the route's Communities.
     */
    public Communities getCommunities() {
    	return communities;
    }

    // -----[ isInternal ]-------------------------------------------
    /**
     * Tests if the route is internal (originated on this router).
     */
    public boolean isInternal() {
    	return internal;
    }

    // -----[ originToString ]---------------------------------------
    /**
     * Converts the given origin to a String.
     */
    public static String originToString(byte bOrigin) {
    	switch (bOrigin) {
    	case ORIGIN_IGP       : return "IGP";
    	case ORIGIN_EGP       : return "EGP";
    	case ORIGIN_INCOMPLETE: return "INCOMPLETE";
    	default:
    		return "DEFAULT";
    	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGP route to a String.
     */
    public String toString() {
    	String s= "";

    	// Flags
    	if (internal)
    		s+= "i";
    	else
    		s+= (feasible?"*":" ");
    	s+= (best?">":" ");
    	s+= " ";

    	// Attributes
    	s+= prefix;
    	s+= "\t";
    	s+= nexthop;
    	s+= "\t";
    	s+= localPref;
    	s+= "\t";
    	s+= med;
    	s+= "\t";
    	if (path != null)
    		s+= path;
    	s+= "\t";
    	switch (origin) {
    	case ORIGIN_IGP: s+= "i"; break;
    	case ORIGIN_EGP: s+= "e"; break;
    	case ORIGIN_INCOMPLETE: s+= "?"; break;
    	default:
    		s+= "?";
    	}
    	if (communities != null) {
    		s+= "\t";
    		s+= communities;
    	}

    	return s;
    }
    
    // -----[ setRank ]---------------------------------------------
    public void setRank(int rank) {
    	properties.setProperty("rank", String.valueOf(rank));
    }
    
    // -----[ getRank ]---------------------------------------------
    public int getRank() {
    	return Integer.parseInt(properties.getProperty("rank"));
    }
    
    // -----[ rankToString ]----------------------------------------
    public static native synchronized String rankToString(int rank);

}
