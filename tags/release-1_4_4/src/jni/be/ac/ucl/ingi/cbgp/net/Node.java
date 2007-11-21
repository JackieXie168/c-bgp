// ==================================================================
// @(#)Node.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/02/2006
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp.net; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.CBGPException;
import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPTrace;

// -----[ Node ]-----------------------------------------------------
/**
 * This class represents a node
 */
public class Node extends Element {

    // -----[ public constants ]-------------------------------------
	public static final String PROTOCOL_ICMP= "icmp";
	public static final String PROTOCOL_IPIP= "ipip";
    public static final String PROTOCOL_BGP = "bgp";
    public static final String PROTOCOL_HLP = "hlp";

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;

    // -----[ Node ]-------------------------------------------------
    /**
     * Node's constructor.
     */
    protected Node(CBGP cbgp, IPAddress address) {
    	super(cbgp);
    	this.address= address;
    }

    // -----[ getAddress ]-------------------------------------------
    /**
     * Returns the node's IP address.
     */
    public IPAddress getAddress() {
    	return address;
    }

    // -----[ getLatitude ]------------------------------------------
    /**
     * Get the latitude of this node.
     */
    public native synchronized float getLatitude()
		throws CBGPException;

    // -----[ setLatitude ]------------------------------------------
    /**
     * Set the latitude of this node.
     */
    public native synchronized void setLatitude(float latitude)
		throws CBGPException;

    // -----[ getLongitude ]-----------------------------------------
    /**
     * Get the longitude of this node.
     */
    public native synchronized float getLongitude()
		throws CBGPException;

    // -----[ setLongitude ]-----------------------------------------
    /**
     * Set the longitude of this node.
     */
    public native synchronized void setLongitude(float longitude)
		throws CBGPException;

    // -----[ getName ]----------------------------------------------
    /**
     * Returns the node's name.
     */
    public native String getName()
    	throws CBGPException;
    
    // -----[ setName ]----------------------------------------------
    /**
     * Set the node's name.
     */
    public native void setName(String name)
    	throws CBGPException;

    // -----[ hasProtocol ]------------------------------------------
    /**
     * Returns 'true' if the given protocol is supported by this
     * node. Use the protocol constants PROTOCOL_XXX defined in this
     * class.
     */
    public native synchronized boolean hasProtocol(String protocol)
    	throws CBGPException;

    // -----[ getProtocols ]-----------------------------------------
    /**
     * Returns the list of protocols supported by this node. The
     * protocols are identified by the protocol constants
     * PROTOCOL_XXX defined in this class.
     */
    public native synchronized Vector<String> getProtocols()
    	throws CBGPException;

    // -----[ getBGP ]-----------------------------------------------
    /**
     * Returns a reference to the BGP router running on this node
     * (if BGP is supported).
     */
    public native synchronized be.ac.ucl.ingi.cbgp.bgp.Router getBGP()
		throws CBGPException;
    
    // -----[ recordRoute ]------------------------------------------
    /**
     * Records the IP-level route from this node towards the given
     * destination address.
     */
    public native synchronized IPTrace recordRoute(String sDstAddr)
		throws CBGPException;

    // -----[ traceRoute ]-------------------------------------------
    /**
     * Traces the IP-level route from this node towards the given
     * destination address.
     */
    public native synchronized IPTrace traceRoute(String sDstAddr)
		throws CBGPException;
    
    // -----[ getAddresses ]-----------------------------------------
    /**
     * Returns the list of addresses supported by this node.
     */
    public native synchronized Vector getAddresses()
    	throws CBGPException;
    
    // -----[ addLTLLink ]------------------------------------------
    /**
     * Adds a loopback-to-loopback (LTL) link to this node.
     */
    public native synchronized Link addLTLLink(Node dst, boolean bidir)
    	throws CBGPException;
    
    // -----[ addPTPLink ]------------------------------------------
    /**
     * Adds a point-to-point (PTP) link to this node.
     */
    public native synchronized Link addPTPLink(Node dst, String siface, 
    		String diface, boolean bidir)
    	throws CBGPException;
    
    // -----[ addPTMPLink ]-----------------------------------------
    /**
     * Adds a point-to-multipoint (PTMP) link to this node. The
     * destination is a Subnet object.
     */
    public native synchronized Link addPTMPLink(Subnet subnet, String siface)
    	throws CBGPException; 

    // -----[ getLinks ]---------------------------------------------
    /**
     * Returns the list of links departing from this node.
     */
    public native synchronized Vector getLinks()
		throws CBGPException;
    
    // -----[ getRT ]------------------------------------------------
    /**
     * Returns the list of routes installed in the routing table
     * of this node.
     */
    public native synchronized Vector getRT(String sPrefix)
		throws CBGPException;

    // -----[ addRoute ]---------------------------------------------
    /**
     * Add a static route to this node.
     *
     * @param sPrefix the route destination prefix
     * @param sNextHop the gateway
     * @param iWeight the route's metric
     */
    public native synchronized
	void addRoute(String sPrefix, String sNexthop, 
			int iWeight)
	    throws CBGPException;
    
    // -----[ loadTraffic ]------------------------------------------
    public native synchronized void loadTraffic(String filename)
    	throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this node to a String.
     */
    public String toString()
    {
    	String s= "";
    	String name= null;

    	s+= address;
	
    	try {
    		name= getName();
    	} catch (CBGPException e) {}
		
    	if (name != null)
			s+= " (name: \""+name+"\")";
		
		return s;
    }

}
