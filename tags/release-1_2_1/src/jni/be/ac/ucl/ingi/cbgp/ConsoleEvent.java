// ==================================================================
// @(#)ConsoleEvent.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/03/2006
// @lastdate 02/03/2006
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
