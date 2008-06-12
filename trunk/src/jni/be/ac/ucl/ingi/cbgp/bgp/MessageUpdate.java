// ==================================================================
// @(#)MessageUpdate.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/06/2007
// @lastdate 20/06/2007
// ==================================================================
package be.ac.ucl.ingi.cbgp.bgp;

import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.net.Message;
import be.ac.ucl.ingi.cbgp.bgp.Route;

// -----[ MessageUpdate class ]--------------------------------------
public class MessageUpdate extends Message
{

    protected Route route;

    // -----[ Constructor ]------------------------------------------
    protected MessageUpdate(IPAddress from, IPAddress to, Route route)
    {
	super(from, to);
	this.route= route;
    }

    // -----[ getRoute ]---------------------------------------------
    public Route getRoute()
    {
	return route;
    }
   
    // -----[ toString ]---------------------------------------------
    public String toString()
    {
	return super.toString()+" UPDATE "+route.getPrefix();
    }

}