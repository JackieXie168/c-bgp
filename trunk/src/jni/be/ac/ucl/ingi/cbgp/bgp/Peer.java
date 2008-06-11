// ==================================================================
// @(#)BGPPeer.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/02/2005
// @lastdate 01/10/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.ProxyObject;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ Peer ]-----------------------------------------------------
/**
 * This class represents a peer of a BGP router.
 */
public class Peer extends ProxyObject
{

    // -----[ constants ]--------------------------------------------
    public static final byte SESSION_STATE_IDLE       = 0;
    public static final byte SESSION_STATE_OPENWAIT   = 1;
    public static final byte SESSION_STATE_ESTABLISHED= 2;
    public static final byte SESSION_STATE_ACTIVE     = 3;

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;
    protected int iAS;

    // -----[ constructor ]-------------------------------------------------
    protected Peer(CBGP cbgp, IPAddress address, int iAS) {
    	super(cbgp);
    	this.address= address;
    	this.iAS= iAS;
    }

    // -----[ getAddress ]-------------------------------------------
    /**
     * Returns the peer's IP address.
     */
    public IPAddress getAddress() {
    	return address;
    }

    // -----[ getAS ]------------------------------------------------
    /**
     * Returns the peer's AS number.
     */
    public int getAS() {
    	return iAS;
    }
    
    // -----[ getRouter ]--------------------------------------------
    /**
     * Returns the BGP router where this peer is defined.
     */
    public native Router getRouter()
    	throws CBGPException;
    
    // -----[ getRouterID ]------------------------------------------
    /**
     * Returns the router-id of the remote peer (received in the
     * OPEN message).
     * 
     * Return value:
     *   if the session is not in ESTABLISHED state, no router-id
     *   has been received. In this case, the method will return a
     *   null objet.
     */
    public native IPAddress getRouterID()
    	throws CBGPException;

    // -----[ getSessionState ]--------------------------------------
    /**
     * Returns the state of the session with this peer.
     */
    public native byte getSessionState()
      throws CBGPException;

    // -----[ openSession ]------------------------------------------
    /**
     * Open the session.
     */
    public native void openSession()
      throws CBGPException;

    // -----[ closeSession ]------------------------------------------
    /**
     * Close the session.
     */
    public native void closeSession()
      throws CBGPException;

    // -----[ sessionStateToString ]---------------------------------
    /**
     * Converts a session state in a String.
     */
    public static String sessionStateToString(byte bSessionState) {
    	switch (bSessionState) {
    	case SESSION_STATE_IDLE: return "IDLE";
    	case SESSION_STATE_OPENWAIT: return "OPENWAIT";
    	case SESSION_STATE_ESTABLISHED: return "ESTABLISHED";
    	case SESSION_STATE_ACTIVE: return "ACTIVE";
    	default:
    		return "?";
    	}
    }
    
    // -----[ recv ]-------------------------------------------------
    public native synchronized	void recv(String sMesg)
	throws CBGPException;
    
    // -----[ isInternal ]-------------------------------------------
    public native synchronized boolean isInternal();

    // -----[ isReflectorClient ]------------------------------------
    /**
     * Returns true if the peer is a route-reflector client.
     */
    public native synchronized boolean isReflectorClient();

    // -----[ setReflectorClient ]-----------------------------------
    /**
     * Configure this peer as a route-reflector client.
     */
    public native synchronized void setReflectorClient()
	throws CBGPException;

    // -----[ isVirtual ]--------------------------------------------
    /**
     * Returns true if the peer is virtual.
     */
    public native synchronized boolean isVirtual();

    // -----[ setVirtual ]-------------------------------------------
    /**
     * Configure this peer as a virtual peer.
     */
    public native synchronized void setVirtual()
	throws CBGPException;

    //  -----[ getNextHopSelf ]--------------------------------------
    public native synchronized boolean getNextHopSelf()
    	throws CBGPException;
    
    // -----[ setNextHopSelf ]--------------------------------------
    public native synchronized void setNextHopSelf(boolean state)
    	throws CBGPException;
    
    // -----[ hasSoftRestart ]---------------------------------------
    /**
     * Returns true if the peer has the soft-restart feature.
     */
    public native synchronized boolean hasSoftRestart()
		throws CBGPException;
    
    // -----[ isAutoConfigured ]------------------------------------
    public native synchronized boolean isAutoConfigured();
    
    // -----[ getNextHop ]-------------------------------------------
    public native synchronized IPAddress getNextHop();
    
    // -----[ getUpdateSource ]--------------------------------------
    public native synchronized IPAddress getUpdateSource();
    
    // -----[ getInputFilter ]---------------------------------------
    public native Filter getInputFilter()
    	throws CBGPException;
    
    // -----[ getOutputFilter ]--------------------------------------
    public native Filter getOutputFilter()
    	throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGP peer to a String.
     */
    public String toString()
    {
	String s= "";
	int iOptions= 0;

	s+= address;
	s+= "\t";
	s+= iAS;
	s+= "\t";
        try {
	  s+= Peer.sessionStateToString(getSessionState());
        } catch (CBGPException e) {
          s+= "???";
        }
	try {
	    if (isReflectorClient()) {
		s+= (iOptions++ > 0)?",":"\t(";
		s+= "rr-client";
	    }
	    if (getNextHopSelf()) {
		s+= (iOptions++ > 0)?",":"\t(";
		s+= "next-hop-self";
	    }
	    if (isVirtual()) {
		s+= (iOptions++ > 0)?",":"\t(";
		s+= "virtual";
	    }
	    if (hasSoftRestart()) {
		s+= (iOptions++ > 0)?",":"\t(";
		s+= "soft-restart";
	    }
	} catch (CBGPException e) {
		s+= "???";
	}
	s+= (iOptions > 0)?")":"";

	return s;
    }

}
