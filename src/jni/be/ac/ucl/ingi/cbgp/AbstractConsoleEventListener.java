// ==================================================================
// @(#)AbstractConsoleEventListener.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 03/03/2006
// $Id: AbstractConsoleEventListener.java,v 1.2 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp;

// -----[ AbstractConsoleEventListener ]-----------------------------
public abstract class AbstractConsoleEventListener
    implements ConsoleEventListener
{

    abstract public void eventFired(ConsoleEvent e);

}
