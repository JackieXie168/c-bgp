// ==================================================================
// @(#)Router.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/03/2006
// @lastdate 02/10/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPPrefix;
import be.ac.ucl.ingi.cbgp.ProxyObject;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.exceptions.InvalidDestinationException;
import be.ac.ucl.ingi.cbgp.net.Node;

// -----[ Router ]--------------------------------------------------
/**
 * This class represents a BGP router
 */
public class Router extends ProxyObject {

	public static enum BGPRouteInputFormat {
		MRT_BINARY("mrt-binary"),
		MRT_ASCII("mrt-ascii"),
		CISCO("cisco");
		private final String value;
		BGPRouteInputFormat(String value) { this.value= value; }
		public String value() { return value; }
	}
	
    // -----[ protected attributes ]---------------------------------
    protected IPAddress routerID;
    protected short asn;
    protected Node node;
    
    // -----[ constructor ]-----------------------------------------
    /**
     * Router's constructor.
     *
     * @param cbgp the C-BGP instance
     * @param the underlying node
     * @param asn the router's AS number
     * @param routerID the router ID
     */
    protected Router(CBGP cbgp, Node node, short asn, IPAddress routerID) {
    	super(cbgp);
    	this.node= node;
    	this.asn= asn;
    	this.routerID= routerID;
    }
    
    // -----[ getNode ]----------------------------------------------
    public Node getNode() {
    	return node;
    }
    
    // -----[ getAddress ]-------------------------------------------
    /**
     * Returns the router's IP address.
     *
     * @return the router's identifier
     */
    public IPAddress getAddress() {
    	return node.getAddress();
    }
    
    // -----[ getASN ]----------------------------------------------
    /**
     * Returns the router's ASN
     *
     * @return the router's ASN
     */
    public short getASN() {
    	return asn;
    }

    // -----[ getRouterID ]-----------------------------------------
    /**
     * Returns the router ID
     *
     * @return the router ID
     */
    public IPAddress getRouterID() {
    	return routerID;
    }

    // -----[ getName ]----------------------------------------------
    /**
     * Returns the router's name.
     *
     * @return the router's name
     */
    public String getName() throws CBGPException {
    	return node.getName();
    }
    
    // -----[ isRouteReflector ]------------------------------------
    public native synchronized boolean isRouteReflector()
    	throws CBGPException;
    
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
    public native Vector<IPPrefix> getNetworks()
    	throws CBGPException;
    
    // -----[ getRIB ]----------------------------------------------
    public native Vector<Route> getRIB(String prefix)
		throws CBGPException, InvalidDestinationException;

    // -----[ getAdjRIB ]-------------------------------------------
    public native Vector<Route> getAdjRIB(String peer, String prefix,
		boolean in)
		throws CBGPException, InvalidDestinationException;

    // -----[ loadRib ]----------------------------------------------
    public native void loadRib(String fileName, boolean force, String type)
		throws CBGPException;

    // -----[ rescan ]-----------------------------------------------
    public native void rescan()
		throws CBGPException;


    // -----[ toString ]---------------------------------------------
    /**
     * Converts this router to a String.
     */
    public String toString() {
    	String s= "";
    	s+= getRouterID();
    	try {
    		s+= " ("+getName()+")";
    	} catch (CBGPException e) {}
    	return s;
    }

}
