// ==================================================================
// @(#)IPPrefix.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 07/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ IPPrefix ]------------------------------------------------
/**
 * This class is a container for an IP prefix.
 */
public class IPPrefix
{

    // -----[ private attributes of the IP prefix ]------------------
    public IPAddress address;
    public int bMask;

    // -----[ IPPrefix ]--------------------------------------------
    /**
     * IPPrefix's constructor.
     */
    public IPPrefix(byte bA, byte bB, byte bC, byte bD, byte bMask)
    {
	this.address= new IPAddress(bA, bB, bC, bD);
	this.bMask= bMask;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this IP prefix to a String.
     */
    public String toString()
    {
	return address+"/"+bMask;
    }

}
