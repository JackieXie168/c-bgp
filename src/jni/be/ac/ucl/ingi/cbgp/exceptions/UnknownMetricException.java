// ==================================================================
// @(#)UnknownMetricException.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/05/2007
// $Id: UnknownMetricException.java,v 1.2 2009-08-31 09:44:07 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.exceptions; 

import java.lang.Exception;

// -----[ UnknownMetricException ]-----------------------------------
/**
 * This class represents an exception.
 */
public class UnknownMetricException extends Exception
{

	private static final long serialVersionUID = 1L;

	// -----[ UnknownMetricException ]-------------------------------
    /**
     * UnknownMetricException's constructor.
     */
    public UnknownMetricException()
    {
    	super("unknown metric");
    }

}
