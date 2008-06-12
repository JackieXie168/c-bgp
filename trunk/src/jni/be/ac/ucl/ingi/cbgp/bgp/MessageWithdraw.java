// ==================================================================
// @(#)MessageWithdraw.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/06/2007
// @lastdate 20/06/2007
// ==================================================================
package be.ac.ucl.ingi.cbgp.bgp;

import be.ac.ucl.ingi.cbgp.IPAddress;
import be.ac.ucl.ingi.cbgp.IPPrefix;
import be.ac.ucl.ingi.cbgp.net.Message;

// -----[ MessageWithdraw class ]------------------------------------
public class MessageWithdraw extends Message
{

    protected IPPrefix prefix;

    // -----[ Constructor ]------------------------------------------
    protected MessageWithdraw(IPAddress from, IPAddress to, IPPrefix prefix)
    {
	super(from, to);
	this.prefix= prefix;
    }

    // -----[ getPrefix ]--------------------------------------------
    public IPPrefix getPrefix()
    {
	return prefix;
    }
   
    // -----[ toString ]---------------------------------------------
    public String toString()
    {
	return super.toString()+" WITHDRAW "+prefix;
    }

}