// ==================================================================
// @(#)ASPath.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/02/2004
// @lastdate 18/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.lang.Exception;
import java.util.Vector;

// -----[ ASPath ]---------------------------------------------------
/**
 * This class represents an AS-Path.
 */
public class ASPath
{
    
    // -----[ protected attributes ]---------------------------------
    protected Vector segments= null;

    // -----[ ASPath ]-----------------------------------------------
    /**
     * ASPath's constructor.
     */
    public ASPath()
    {
	this.segments= new Vector();
    }

    // -----[ append ]--------------------------------------------------
    /**
     * Append a new AS-path segment.
     */
    public void append(ASPathSegment segment)
    {
	segments.add(segment);
    }

    // -----[ getSegment ]-------------------------------------------
    /**
     * Return the segment at the given position.
     */
    public ASPathSegment getSegment(int iIndex) throws Exception
    {
	if ((iIndex < 0) || (iIndex >= segments.size()))
	    throw new Exception("Invalid AS-path segment index "+iIndex);
	return (ASPathSegment) segments.get(iIndex);
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the AS-Path to a String.
     */
    public String toString()
    {
	String s= "";

	for (int iIndex= 0; iIndex < segments.size(); iIndex++) {
	    if (iIndex > 0) {
		s+= " ";
	    }
	    s+= (ASPathSegment) segments.get(iIndex);
	}
	return s;
    }

}
