// ==================================================================
// @(#)ASPath.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2004
// $Id: ASPath.java,v 1.4 2009-08-31 09:42:43 bqu Exp $
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
    
    // -----[ getSegmentCount ]--------------------------------------
    /**
     * Return the number of segments.
     */
    public int getSegmentCount() {
    	return segments.size();
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
