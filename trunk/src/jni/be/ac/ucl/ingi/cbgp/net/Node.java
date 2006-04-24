// ==================================================================
// @(#)Node.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/02/2006
// @lastdate 24/04/2006
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
    protected String name;
    protected Hashtable protocols;

    // -----[ Node ]-------------------------------------------------
    /**
     * Node's constructor.
     */
    public Node(CBGP cbgp, IPAddress address, String name, Hashtable protocols)
    {
    	super(cbgp);
    	this.address= address;
    	this.name= name;
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

    // -----[ getName ]----------------------------------------------
    /**
     * Returns the node's name.
     */
    public String getName()
    {
	return name;
    }

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
    
    // -----[ recordRoute ]-----------------------------------------
    public native synchronized 	IPTrace recordRoute(String sDstAddr)
		throws CBGPException;
    
    // -----[ getAddresses ]----------------------------------------
    public native synchronized Vector getAddresses()
    	throws CBGPException;
    
    // -----[ getLinks ]--------------------------------------------
    public native synchronized Vector getLinks()
		throws CBGPException;
    
    // -----[ getRT ]-----------------------------------------
    public native synchronized Vector getRT(String sPrefix)
		throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this node to a String.
     */
    public String toString()
    {
	String s= "";

	s+= address;
	if (name != null)
	    s+= " (name: \""+name+"\")";

	return s;
    }

}
