// ==================================================================
// @(#)ASPath.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/02/2004
// @lastdate 19/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.lang.Exception;
import java.util.Vector;

// -----[ ASPath ]---------------------------------------------------
/**
 * This class represents an AS-Path.
 */
public class ASPath {
    
    // -----[ protected attributes ]---------------------------------
    protected Vector<ASPathSegment> segments= null;

    // -----[ ASPath ]-----------------------------------------------
    /**
     * ASPath's constructor.
     */
    public ASPath() {
    	this.segments= new Vector<ASPathSegment>();
    }

    // -----[ append ]--------------------------------------------------
    /**
     * Append a new AS-path segment.
     */
    public void append(ASPathSegment segment) {
    	segments.add(segment);
    }

    // -----[ getSegment ]-------------------------------------------
    /**
     * Return the segment at the given position.
     */
    public ASPathSegment getSegment(int iIndex)
    	throws Exception {
    	if ((iIndex < 0) || (iIndex >= segments.size()))
    		throw new Exception("Invalid AS-path segment index "+iIndex);
    	return (ASPathSegment) segments.get(iIndex);
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the AS-Path to a String.
     */
    public String toString() {
    	String s= "";

    	for (int iIndex= segments.size(); iIndex > 0; iIndex--) {
    		if (iIndex < segments.size()) {
    			s+= " ";
    		}
    		s+= segments.get(iIndex-1);
    	}
    	return s;
    }
    
}
