// ==================================================================
// @(#)BGPRoute.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 18/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ BGPRoute ]-------------------------------------------------
/**
 * This class is a container for BGP route information.
 */
public class BGPRoute extends Route
{

    // -----[ public constants ]-------------------------------------
    public static final byte ROUTE_ORIGIN_IGP= 0;
    public static final byte ROUTE_ORIGIN_EGP= 1;
    public static final byte ROUTE_ORIGIN_INCOMPLETE= 2;

    // -----[ private attributes of the route ]----------------------
    protected long lLocalPref;
    protected long lMED;
    protected byte bOrigin;
    protected ASPath path;

    // -----[ BGPRoute ]---------------------------------------------
    /**
     * BGPRoute's constructor. Builds a BGP route with its
     * attributes. Currently only supports destination prefix and
     * next-hop.
     */
    public BGPRoute(IPPrefix prefix, IPAddress nexthop, long lLocalPref, long lMED,
		    boolean bBest, boolean bFeasible, byte bOrigin,
		    ASPath path)
    {
	super(prefix, nexthop, bBest, bFeasible);

	// Attributes
	this.lLocalPref= lLocalPref;
	this.lMED= lMED;
	this.bOrigin= bOrigin;
	this.path= path;
    }

    // -----[ getLocalPref ]-----------------------------------------
    /**
     * Returns the route's local-preference.
     */
    public long getLocalPref()
    {
	return lLocalPref;
    }

    // -----[ getMED ]-----------------------------------------------
    /**
     * Returns the routes's MED.
     */
    public long getMED()
    {
	return lMED;
    }

    // -----[ getOrigin ]--------------------------------------------
    /**
     * Returns the route's origin.
     */
    public byte getOrigin()
    {
	return bOrigin;
    }

    // -----[ getPath ]----------------------------------------------
    /**
     * Returns the route's AS-Path.
     */
    public ASPath getPath()
    {
	return path;
    }

    // -----[ originToString ]---------------------------------------
    /**
     * Converts the given origin to a String.
     */
    public static String originToString(byte bOrigin)
    {
	switch (bOrigin) {
	case ROUTE_ORIGIN_IGP: return "IGP";
	case ROUTE_ORIGIN_EGP: return "EGP";
	case ROUTE_ORIGIN_INCOMPLETE: return "INCOMPLETE";
	default:
	    return "DEFAULT";
	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGP route to a String.
     */
    public String toString()
    {
	String s= "";

	// Flags
	s+= (bFeasible?"*":" ");
	s+= (bBest?">":" ");
	s+= " ";

	// Attributes
	s+= prefix;
	s+= "\t";
	s+= nexthop;
	s+= "\t";
	s+= lLocalPref;
	s+= "\t";
	s+= lMED;
	s+= "\t";
	if (path != null) {
	    s+= path;
	}
	s+= "\t";
	switch (bOrigin) {
	case ROUTE_ORIGIN_IGP: s+= "i"; break;
	case ROUTE_ORIGIN_EGP: s+= "e"; break;
	case ROUTE_ORIGIN_INCOMPLETE: s+= "?"; break;
	default:
	    s+= "?";
	}

	return s;
    }

}
