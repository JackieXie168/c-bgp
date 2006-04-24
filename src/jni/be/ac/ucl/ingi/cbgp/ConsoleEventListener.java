// ==================================================================
// @(#)ConsoleEventListener.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/03/2006
// @lastdate 02/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.EventListener;

// -----[ ConsoleEventListener ]-------------------------------------
public interface ConsoleEventListener extends EventListener
{

    public void eventFired(ConsoleEvent e);

}
