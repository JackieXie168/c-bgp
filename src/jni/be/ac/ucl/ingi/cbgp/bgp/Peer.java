// ==================================================================
// @(#)BGPPeer.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/02/2005
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp;

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.*;

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

    public static final byte PEER_FLAG_RR_CLIENT    = 0x01;
    public static final byte PEER_FLAG_VIRTUAL      = 0x04;
    public static final byte PEER_FLAG_SOFT_RESTART = 0x08;

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;
    protected int iAS;
    protected byte bFlags;

    // -----[ Peer ]-------------------------------------------------
    /**
     * Peer's constructor.
     */
    protected Peer(CBGP cbgp, IPAddress address, int iAS, byte bFlags)
    {
    	super(cbgp);
    	this.address= address;
    	this.iAS= iAS;
    	this.bFlags= bFlags;
    }

    // -----[ getAddress ]-------------------------------------------
    /**
     * Returns the peer's IP address.
     */
    public IPAddress getAddress()
    {
	return address;
    }

    // -----[ getAS ]------------------------------------------------
    /**
     * Returns the peer's AS number.
     */
    public int getAS()
    {
	return iAS;
    }
    
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
    public static String sessionStateToString(byte bSessionState)
    {
	switch (bSessionState) {
	case SESSION_STATE_IDLE: return "IDLE";
	case SESSION_STATE_OPENWAIT: return "OPENWAIT";
	case SESSION_STATE_ESTABLISHED: return "ESTABLISHED";
	case SESSION_STATE_ACTIVE: return "ACTIVE";
	default:
	    return "?";
	}
    }
    
    // -----[ recv ]------------------------------------------------
    public native synchronized	void bgpRouterPeerRecv(String sMesg)
	    throws CBGPException;
    
    // -----[ getAdjRib ]-------------------------------------------
    public native Vector getAdjRib(String sPrefix, boolean bIn)
		throws CBGPException;
    
    // -----[ getFlags ]---------------------------------------------
    protected native synchronized boolean getFlags()
    	throws CBGPException;

    // -----[ isInternal ]-------------------------------------------
    public native synchronized boolean isInternal()
	throws CBGPException;

    // -----[ isReflectorClient ]------------------------------------
    /**
     * Returns true if the peer is a route-reflector client.
     */
    public boolean isReflectorClient()
    {
    	return (bFlags & PEER_FLAG_RR_CLIENT) != 0;
    }

    // -----[ isVirtual ]--------------------------------------------
    /**
     * Returns true if the peer is virtual.
     */
    public boolean isVirtual()
    {
    	return (bFlags & PEER_FLAG_VIRTUAL) != 0;
    }

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
    public boolean hasSoftRestart()
    {
	return (bFlags & PEER_FLAG_SOFT_RESTART) != 0;
    }
    
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
	if (isReflectorClient()) {
	    s+= (iOptions++ > 0)?",":"\t(";
	    s+= "rr-client";
	}
	try {
		if (getNextHopSelf()) {
			s+= (iOptions++ > 0)?",":"\t(";
			s+= "next-hop-self";
		}
	} catch (CBGPException e) {
		s+= "???";
	}
	if (isVirtual()) {
	    s+= (iOptions++ > 0)?",":"\t(";
	    s+= "virtual";
	}
	if (hasSoftRestart()) {
	    s+= (iOptions++ > 0)?",":"\t(";
	    s+= "soft-restart";
	}
	s+= (iOptions > 0)?")":"";

	return s;
    }

}
