// ==================================================================
// @(#)BGPDomain.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/03/2006
// @lastdate 20/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.CBGPException;
import be.ac.ucl.ingi.cbgp.ProxyObject;

// -----[ BGPDomain ]------------------------------------------------
/**
 * This class represents an AS (BGP domain).
 */
public class Domain extends ProxyObject
{

    // -----[ protected attributes ]---------------------------------
    protected int id;

    // -----[ BGPDomain ]--------------------------------------------
    /**
     * BGPDomain's constructor.
     */
    protected Domain(CBGP cbgp, int id)
    {
    	super(cbgp);
    	this.id= id;
    }

    // -----[ getID ]------------------------------------------------
    /**
     * Returns the domain's ID.
     */
    public int getID()
    {
	return id;
    }
    
    // -----[ getRouters ]------------------------------------------
    public native synchronized	Vector getRouters()
		throws CBGPException;
    
    //  -----[ rescan ]-------------------------------------------------
    public native void rescan()
		throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGPDomain to a String.
     */
    public String toString()
    {
	String s= "";

	s+= "AS "+id;

	return s;
    }

}
