// ==================================================================
// @(#)Link.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/02/2005
// $Id: Link.java,v 1.7 2009-08-31 09:44:52 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.net; 

import be.ac.ucl.ingi.cbgp.*;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ Link ]-----------------------------------------------------
/**
 * This class is a container for a link.
 */
public class Link extends ProxyObject {
	
	public static final long MAX_METRIC= 4294967295l;

    // -----[ protected attributes ]---------------------------------
    protected IPPrefix id; // Interface address / mask
    protected long lDelay;

    // -----[ Proxy management native methods ]----------------------
    private native void _proxy_finalize();

    // -----[ Link ]-------------------------------------------------
    /**
     * Link's constructor.
     */
    protected Link(CBGP cbgp, IPPrefix id, long lDelay) {
    	super(cbgp);
    	this.id= id;
    	this.lDelay= lDelay;
    }

    // -----[ finalize ]---------------------------------------------
    protected void finalize() {
    	_proxy_finalize();
    }

    // -----[ getCapacity ]------------------------------------------
    public native synchronized long getCapacity()
    	throws CBGPException;
    
    // -----[ setCapacity ]------------------------------------------
    public native synchronized void setCapacity(long capacity)
    	throws CBGPException;
    
    // -----[ getDelay ]---------------------------------------------
    /**
     * Return the link's propagation delay.
     */
    public long getDelay() {
    	return lDelay;
    }
    
    // -----[ setDelay ]---------------------------------------------
    
    // -----[ getLoad ]----------------------------------------------
    /**
     * Return the link's traffic load.
     */
    public native synchronized long getLoad()
    	throws CBGPException;
    
    // -----[ setLoad ]---------------------------------------------- 

    // -----[ getId ]-----------------------------------------------
    /**
     * Return the interface's ID (address / mask).
     */
    public IPPrefix getId() {
    	return id;
    }
    
    // -----[ getWeight ]--------------------------------------------
    /**
     * Return the link's IGP weight.
     */
    public native synchronized long getWeight()
    	throws CBGPException;
    
    // -----[ setWeight ]-------------------------------------------
    /**
     * Set the link's IGP weight.
     */
    public native synchronized	void setWeight(long lWeight)
		throws CBGPException;

    // -----[ getState ]---------------------------------------------
    /**
     *
     */
    public native synchronized boolean getState()
    	throws CBGPException;

    // -----[ setState ]---------------------------------------------
    /**
     *
     */
    public native synchronized void setState(boolean state)
		throws CBGPException;
    
    // -----[ getHead ]---------------------------------------------
    public native synchronized Node getHead()
    	throws CBGPException;
    
    // -----[ getTail ]----------------------------------------------
    public native synchronized Element getTail()
    	throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Convert this link to a String.
     */
    public String toString()
    {
	String s= "";

	/* Attributes */
	s+= id.address+"/"+id.bMask;
	s+= "\t";
	s+= lDelay;
	s+= "\t";
	try {
		s+= getWeight();
	} catch (CBGPException e) {
		s+= "???";
	}
	s+= "\t";

	/* Flags */
	try {
	    s+= (getState()?"UP":"DOWN");
	} catch (CBGPException e) {
	    s+= "???";
	}
	s+= "\t";

	s+= "DIRECT";

	return s;
    }

}
