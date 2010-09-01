// ==================================================================
// @(#)BGPMsgListener.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/06/2007
// $Id: BGPMsgListener.java,v 1.2 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import be.ac.ucl.ingi.cbgp.net.Message;

// -----[ BGPMsgListener ]-------------------------------------------
public interface BGPMsgListener
{

    public void handleMessage(Message m);

}
