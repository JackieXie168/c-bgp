// ==================================================================
// @(#)BGPPeer.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 11/02/2005
// @lastdate 11/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ BGPPeer ]--------------------------------------------------
/**
 * This class represents a peer of a BGP router.
 */
public class BGPPeer
{

    // -----[ constants ]--------------------------------------------
    public static final byte SESSION_STATE_IDLE= 0;
    public static final byte SESSION_STATE_OPENWAIT= 1;
    public static final byte SESSION_STATE_ESTABLISHED= 2;
    public static final byte SESSION_STATE_ACTIVE= 3;

    // -----[ protected attributes ]---------------------------------
    protected IPAddress address;
    protected int iAS;
    protected byte bSessionState;

    // -----[ BGPPeer ]----------------------------------------------
    /**
     * BGPPeer's constructor.
     */
    public BGPPeer(IPAddress address, int iAS, byte bSessionState)
    {
	this.address= address;
	this.iAS= iAS;
	this.bSessionState= bSessionState;
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

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGP peer to a String.
     */
    public String toString()
    {
	String s= "";

	s+= address;
	s+= "\t";
	s+= iAS;
	s+= "\t";
	s+= BGPPeer.sessionStateToString(bSessionState);

	return s;
    }

}
