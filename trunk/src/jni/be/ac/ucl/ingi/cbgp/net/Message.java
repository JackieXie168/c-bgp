// ==================================================================
// @(#)Message.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/06/2007
// $Id: Message.java,v 1.2 2009-08-31 09:44:52 bqu Exp $
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