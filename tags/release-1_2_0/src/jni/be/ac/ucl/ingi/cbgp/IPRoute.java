// ==================================================================
// @(#)IPRoute.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/02/2005
// @lastdate 08/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ IPRoute ]--------------------------------------------------
/**
 * This class is a container for IP route information.
 */
public class IPRoute extends Route
{

    // -----[ public constants ]-------------------------------------
    public static final byte NET_ROUTE_STATIC= 0x01;
    public static final byte NET_ROUTE_IGP= 0x02;
    public static final byte NET_ROUTE_BGP= 0x04;

    // -----[ protected attributes of the IP route ]-----------------
    protected byte bType;

    // -----[ IPRoute ]----------------------------------------------
    /**
     * IPRoute's constructor.
     */
    public IPRoute(IPPrefix prefix, IPAddress nexthop, byte bType)
    {
	super(prefix, nexthop, true, true);

	/* Attributes */
	this.bType= bType;
    }

    // -----[ getType ]----------------------------------------------
    /**
     * Returns the route's type (i.e. the protocol that added the
     * route).
     */
    public byte getType()
    {
	return bType;
    }

    // -----[ typeToString ]-----------------------------------------
    /**
     * Returns a String version of the given route type.
     */
    public static String typeToString(byte bType)
    {
	switch (bType) {
	case NET_ROUTE_STATIC: return "STATIC";
	case NET_ROUTE_IGP: return "IGP";
	case NET_ROUTE_BGP: return "BGP";
	default:
	    return "UNKNOWN";
	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * This function converts this route in a String.
     */
    public String toString()
    {
	String s= "";

	/* Flags */
	s+= (bFeasible?"*":" ");
	s+= (bBest?">":" ");
	s+= " ";

	/* Attributes */
	s+= prefix;
	s+= "\t";
	s+= nexthop;
	s+= "\t";
	s+= typeToString(bType);

	return s;
    }

}
