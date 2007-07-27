// ==================================================================
// @(#)IPAddress.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 19/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ IPAddress ]------------------------------------------------
/**
 * 
 */
public class IPAddress
{

    // -----[ private attributes of the address ]--------------------
    private byte [] abAddress= null;

    // -----[ IPAddress ]--------------------------------------------
    /**
     * IPAddress's constructor.
     */
    public IPAddress(byte bA, byte bB, byte bC, byte bD)
    {
	abAddress= new byte[4];
	abAddress[0]= bA;
	abAddress[1]= bB;
	abAddress[2]= bC;
	abAddress[3]= bD;
    }

    // -----[ byte2int ]---------------------------------------------
    /**
     * Convert a signed byte (Java) to an unsigned byte stored in an
     * integer. We need this to correctly handle bytes coming from the
     * C part of C-BGP and that are unsigned bytes.
     */
    protected static int byte2int(byte b)
    {
	return (b >= 0)?b:(256+b);
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this IP address to a String.
     */
    public String toString()
    {
	return byte2int(abAddress[0])+"."+
	    byte2int(abAddress[1])+"."+
	    byte2int(abAddress[2])+"."+
	    byte2int(abAddress[3]);
    }

    // -----[ equals ]-----------------------------------------------
    public boolean equals(IPAddress addr)
    {
	return addr.toString().equals(toString());
    }

}
