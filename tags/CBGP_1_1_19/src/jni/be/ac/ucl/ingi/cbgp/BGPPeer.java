// ==================================================================
// @(#)BGPPeer.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/02/2005
// @lastdate 18/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ BGPPeer ]--------------------------------------------------
/**
 * This class represents a peer of a BGP router.
 */
public class BGPPeer
{

    // -----[ constants ]--------------------------------------------
    public static final byte SESSION_STATE_IDLE       = 0;
    public static final byte SESSION_STATE_OPENWAIT   = 1;
    public static final byte SESSION_STATE_ESTABLISHED= 2;
    public static final byte SESSION_STATE_ACTIVE     = 3;

    public static final byte PEER_FLAG_RR_CLIENT    = 0x01;
    public static final byte PEER_FLAG_NEXT_HOP_SELF= 0x02;
    public static final byte PEER_FLAG_VIRTUAL      = 0x04;
    public static final byte PEER_FLAG_SOFT_RESTART = 0x08;

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;
    protected int iAS;
    protected byte bSessionState;
    protected byte bFlags;

    // -----[ BGPPeer ]----------------------------------------------
    /**
     * BGPPeer's constructor.
     */
    public BGPPeer(IPAddress address, int iAS, byte bSessionState,
		   byte bFlags)
    {
	this.address= address;
	this.iAS= iAS;
	this.bSessionState= bSessionState;
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

    // -----[ getSessionState ]--------------------------------------
    /**
     * Returns the state of the session with this peer.
     */
    public byte getSessionState()
    {
	return bSessionState;
    }

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

    // -----[ hasNextHopSelf ]---------------------------------------
    /**
     * Returns true if the peer has the next-hop-self feature.
     */
    public boolean hasNextHopSelf()
    {
	return (bFlags & PEER_FLAG_NEXT_HOP_SELF) != 0;
    }

    // -----[ hasSoftRestart ]---------------------------------------
    /**
     * Returns true if the peer has the soft-restart feature.
     */
    public boolean hasSoftRestart()
    {
	return (bFlags & PEER_FLAG_SOFT_RESTART) != 0;
    }

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
	s+= BGPPeer.sessionStateToString(bSessionState);
	if (isReflectorClient()) {
	    s+= (iOptions++ > 0)?",":"\t(";
	    s+= "rr-client";
	}
	if (hasNextHopSelf()) {
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
	s+= (iOptions > 0)?")":"";

	return s;
    }

}
