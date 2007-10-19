// ==================================================================
// @(#)Node.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/02/2006
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp.net; 

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.CBGPException;
import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPTrace;
import be.ac.ucl.ingi.cbgp.ProxyObject;

// -----[ Node ]-----------------------------------------------------
/**
 * This class represents a node
 */
public class Node extends ProxyObject
{

    // -----[ public constants ]-------------------------------------
    public static final String PROTOCOL_BGP= "bgp";
    public static final String PROTOCOL_HLP= "hlp";

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;
    protected Hashtable protocols;

    // -----[ Node ]-------------------------------------------------
    /**
     * Node's constructor.
     */
    public Node(CBGP cbgp, IPAddress address, Hashtable protocols)
    {
    	super(cbgp);
    	this.address= address;
    	this.protocols= protocols;
    }

    // -----[ getAddress ]-------------------------------------------
    /**
     * Returns the node's IP address.
     */
    public IPAddress getAddress()
    {
	return address;
    }

    // -----[ getLatitude ]------------------------------------------
    public native synchronized float getLatitude()
	throws CBGPException;

    // -----[ setLatitude ]------------------------------------------
    public native synchronized void setLatitude(float latitude)
	throws CBGPException;

    // -----[ getLongitude ]-----------------------------------------
    public native synchronized float getLongitude()
	throws CBGPException;

    // -----[ setLongitude ]-----------------------------------------
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
     * Returns 'true' if the given protocol is supported by this node.
     */
    public boolean hasProtocol(String protocol)
    {
	if (protocols != null) {
	    return protocols.containsKey(protocol);
	}
	return false;
    }

    // -----[ getProtocols ]-----------------------------------------
    public Enumeration getProtocols()
    {
	if (protocols != null)
	    return protocols.keys();
	return null;
    }

    // -----[ getBGP ]-----------------------------------------------
    public native synchronized be.ac.ucl.ingi.cbgp.bgp.Router getBGP()
	throws CBGPException;
    
    // -----[ recordRoute ]------------------------------------------
    public native synchronized IPTrace recordRoute(String sDstAddr)
		throws CBGPException;

    // -----[ traceRoute ]-------------------------------------------
    public native synchronized IPTrace traceRoute(String sDstAddr)
		throws CBGPException;
    
    // -----[ getAddresses ]-----------------------------------------
    public native synchronized Vector getAddresses()
    	throws CBGPException;
    
    // -----[ getLinks ]---------------------------------------------
    public native synchronized Vector getLinks()
		throws CBGPException;
    
    // -----[ getRT ]------------------------------------------------
    public native synchronized Vector getRT(String sPrefix)
		throws CBGPException;

    // -----[ addRoute ]---------------------------------------------
    /**
     * Add a static route to a node.
     *
     * @param sPrefix the route destination prefix
     * @param sNextHop the gateway
     * @param iWeight the route's metric
     */
    public native synchronized
	void addRoute(String sPrefix, String sNexthop, 
		      int iWeight)
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
