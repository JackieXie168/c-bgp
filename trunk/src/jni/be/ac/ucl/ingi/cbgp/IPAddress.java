// ==================================================================
// @(#)IPAddress.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 07/02/2005
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

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this IP address to a String.
     */
    public String toString()
    {
	return abAddress[0]+"."+abAddress[1]+"."+abAddress[2]+"."+abAddress[3];
    }

}
