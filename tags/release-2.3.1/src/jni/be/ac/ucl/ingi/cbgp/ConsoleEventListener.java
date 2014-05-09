// ==================================================================
// @(#)ConsoleEventListener.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/03/2006
// $Id: ConsoleEventListener.java,v 1.2 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.EventListener;

// -----[ ConsoleEventListener ]-------------------------------------
public interface ConsoleEventListener extends EventListener
{

    public void eventFired(ConsoleEvent e);

}
