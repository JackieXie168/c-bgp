// ==================================================================
// @(#)AbstractConsoleEventListener.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 03/03/2006
// @lastdate 03/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp;

// -----[ AbstractConsoleEventListener ]-----------------------------
public abstract class AbstractConsoleEventListener
    implements ConsoleEventListener
{

    abstract public void eventFired(ConsoleEvent e);

}
