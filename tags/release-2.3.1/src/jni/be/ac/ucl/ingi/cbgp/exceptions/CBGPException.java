// ==================================================================
// @(#)CBGPException.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// $Id: CBGPException.java,v 1.2 2009-08-31 09:44:07 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.exceptions; 

import java.lang.Exception;

// -----[ CBGPException ]--------------------------------------------
/**
 * This class represents an exception related to C-BGP.
 */
@SuppressWarnings("serial")
public class CBGPException extends Exception
{

	// -----[ CBGPException ]----------------------------------------
    /**
     * CBGPException's constructor.
     */
    public CBGPException(String sMsg)
    {
    	super(sMsg);
    }

}
