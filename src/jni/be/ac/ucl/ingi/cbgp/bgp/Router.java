// ==================================================================
// @(#)Router.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/03/2006
// @lastdate 29/06/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.CBGPException;
import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPPrefix;
import be.ac.ucl.ingi.cbgp.ProxyObject;

// -----[ Router ]--------------------------------------------------
/**
 * This class represents a BGP router
 */
public class Router extends ProxyObject
{

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;
    protected IPAddress routerID;
    protected short asn;
    protected String name;

    // -----[ Router ]----------------------------------------------
    /**
     * Router's constructor.
     */
    protected Router(CBGP cbgp, IPAddress address, short asn, IPAddress routerID, String name)
    {
    	super(cbgp);
    	this.address= address;
    	this.asn= asn;
    	this.routerID= routerID;
    	this.name= name;
    }
    
    // -----[ getAddress ]-------------------------------------------
    /**
     * Returns the router's IP address.
     */
    public IPAddress getAddress()
    {
    	return address;
    }
    
    // -----[ getASN ]----------------------------------------------
    public short getASN()
    {
    	return asn;
    }

    // -----[ getRouterID ]-----------------------------------------
    public IPAddress getRouterID()
    {
    	return routerID;
    }

    // -----[ getName ]----------------------------------------------
    /**
     * Returns the router's name.
     */
    public String getName()
    {
	return name;
    }
    
    // -----[ addNetwork ]------------------------------------------
    public native synchronized	IPPrefix addNetwork(String sPrefix)
		throws CBGPException;
    
    // -----[ delNetwork ]------------------------------------------
    public native synchronized void delNetwork(String sPrefix)
    	throws CBGPException;
    
    // -----[ addPeer ]---------------------------------------------
    public native synchronized	Peer addPeer(String sPeerAddr, int iASNumber)
		throws CBGPException;
    
    // -----[ getPeers ]--------------------------------------------
    public native Vector<Peer> getPeers()
		throws CBGPException;
    
    // -----[ getNetworks ]-----------------------------------------
    public native Vector getNetworks()
    	throws CBGPException;
    
    // -----[ getRIB ]----------------------------------------------
    public native Vector<Route> getRIB(String prefix)
	throws CBGPException;

    // -----[ getAdjRIB ]-------------------------------------------
    public native Vector<Route> getAdjRIB(String peer, String prefix,
					  boolean in)
	throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this router to a String.
     */
    public String toString()
    {
	String s= "";

	s+= address;
	s+= ":"+asn;
	if (name != null)
	    s+= " (name: \""+name+"\")";

	return s;
    }

}
