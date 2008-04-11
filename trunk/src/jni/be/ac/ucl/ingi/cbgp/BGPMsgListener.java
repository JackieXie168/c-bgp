// ==================================================================
// @(#)BGPMsgListener.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 19/06/2007
// @lastdate 20/06/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import be.ac.ucl.ingi.cbgp.net.Message;

// -----[ BGPMsgListener ]-------------------------------------------
public interface BGPMsgListener
{

    public void handleMessage(Message m);

}
