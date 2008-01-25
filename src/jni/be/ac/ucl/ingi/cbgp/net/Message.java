// ==================================================================
// @(#)Message.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/06/2007
// @lastdate 20/06/2007
// ==================================================================
package be.ac.ucl.ingi.cbgp.net;

import be.ac.ucl.ingi.cbgp.IPAddress;

// -----[ Message class ]--------------------------------------------
public class Message
{

    protected IPAddress from;
    protected IPAddress to;

    // -----[ Constructor ]------------------------------------------
    protected Message(IPAddress from, IPAddress to)
    {
	this.from= from;
	this.to= to;
    }

    // -----[ toString ]---------------------------------------------
    public String toString()
    {
	return "("+from+","+to+")";
    }
    
    // -----[ getFrom ]---------------------------------------------
    public IPAddress getFrom() {
    	return from;
    }

    // -----[ getTo ]-----------------------------------------------
    public IPAddress getTo() {
    	return to;
    }
    
}