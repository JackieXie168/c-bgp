// ==================================================================
// @(#)UnknownMetricException.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/05/2007
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.lang.Exception;

// -----[ UnknownMetricException ]-----------------------------------
/**
 * This class represents an exception.
 */
public class UnknownMetricException extends Exception
{

    // -----[ UnknownMetricException ]-------------------------------
    /**
     * UnknownMetricException's constructor.
     */
    public UnknownMetricException()
    {
    	super("unknown metric");
    }

}
