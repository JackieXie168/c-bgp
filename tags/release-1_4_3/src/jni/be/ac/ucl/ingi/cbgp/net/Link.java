// ==================================================================
// @(#)Link.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/02/2005
// @lastdate 31/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp.net; 

import be.ac.ucl.ingi.cbgp.*;

// -----[ Link ]-----------------------------------------------------
/**
 * This class is a container for a link.
 */
public class Link extends ProxyObject
{

    // -----[ protected attributes ]---------------------------------
    protected IPAddress nexthopIf;
    protected long lDelay;

    // -----[ Proxy management native methods ]----------------------
    private native void _proxy_finalize();

    // -----[ Link ]-------------------------------------------------
    /**
     * Link's constructor.
     */
    protected Link(CBGP cbgp, IPAddress nexthopIf, long lDelay)
    {
    	super(cbgp);
    	/* Attributes */
    	this.nexthopIf= nexthopIf;
    	this.lDelay= lDelay;
    }

    // -----[ finalize ]---------------------------------------------
    protected void finalize()
    {
	_proxy_finalize();
    }

    // -----[ getDelay ]---------------------------------------------
    /**
     * Return the link's propagation delay.
     */
    public long getDelay()
    {
	return lDelay;
    }

    // -----[ getNexthopIf ]-----------------------------------------
    /**
     * Return the link's nexthop interface.
     */
    public IPAddress getNexthopIf()
    {
	return nexthopIf;
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

    // -----[ toString ]---------------------------------------------
    /**
     * Convert this link to a String.
     */
    public String toString()
    {
	String s= "";

	/* Attributes */
	s+= nexthopIf;
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
