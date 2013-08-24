// ==================================================================
// @(#)ConsoleEvent.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/03/2006
// $Id: ConsoleEvent.java,v 1.2 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ ConsoleEventListener ]-------------------------------------
public class ConsoleEvent
{

    private String msg= null;
    
    // -----[ ConsoleEvent constructor ]-----------------------------
    public ConsoleEvent(String msg)
    {
	this.msg= msg;
    }

    // -----[ getMessage ]-------------------------------------------
    public String getMessage()
    {
	return msg;
    }
    
}
