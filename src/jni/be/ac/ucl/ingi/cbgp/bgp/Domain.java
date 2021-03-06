// ==================================================================
// @(#)BGPDomain.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/03/2006
// $Id: Domain.java,v 1.5 2009-08-31 09:42:43 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.ProxyObject;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ BGPDomain ]------------------------------------------------
/**
 * This class represents an AS (BGP domain).
 */
public class Domain extends ProxyObject {

    // -----[ protected attributes ]---------------------------------
    protected int id;

    // -----[ BGPDomain ]--------------------------------------------
    /**
     * BGPDomain's constructor.
     */
    protected Domain(CBGP cbgp, int id) {
    	super(cbgp);
    	this.id= id;
    }

    // -----[ getID ]------------------------------------------------
    /**
     * Returns the domain's ID.
     */
    public int getID() {
    	return id;
    }
    
    // -----[ getRouters ]------------------------------------------
    public native synchronized Vector<Router> getRouters()
		throws CBGPException;
    
    //  -----[ rescan ]-------------------------------------------------
    public native void rescan()
		throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGPDomain to a String.
     */
    public String toString() {
    	return "AS "+id;
    }

}
