// ==================================================================
// @(#)CBGPException.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 07/02/2005
// @lastdate 21/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp.exceptions; 

import java.lang.Exception;

// -----[ CBGPException ]--------------------------------------------
/**
 * This class represents an exception related to C-BGP.
 */
public class CBGPException extends Exception
{

    private static final long serialVersionUID = 1L;

	// -----[ CBGPException ]----------------------------------------
    /**
     * CBGPException's constructor.
     */
    public CBGPException(String sMsg)
    {
    	super(sMsg);
    }

}
