// ==================================================================
// @(#)ASPathSegment.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/02/2004
// @lastdate 18/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.lang.Exception;
import java.util.Vector;

// -----[ ASPathSegment ]--------------------------------------------
/**
 * This class represents an AS-Path segment.
 */
public class ASPathSegment
{

    // -----[ constants ]--------------------------------------------
    public static final int AS_PATH_SEGMENT_SET     = 1;
    public static final int AS_PATH_SEGMENT_SEQUENCE= 2;

    // -----[ protected attributes ]---------------------------------
    protected int iType;
    protected Vector ases= null;

    // -----[ ASPathSegment ]----------------------------------------
    /**
     * ASPathSegment's constructor.
     */
    public ASPathSegment(int iType) throws Exception
    {
	this.ases= new Vector();
	if ((iType != AS_PATH_SEGMENT_SET) &&
	    (iType != AS_PATH_SEGMENT_SEQUENCE))
	    throw new Exception("Invalid AS-Path segment type "+iType);
	this.iType= iType;
    }

    // -----[ append ]-----------------------------------------------
    /**
     * Append an AS-number to the AS-Path segment.
     */
    public void append(int iAS) throws Exception
    {
	if ((iAS < 0) || (iAS > 65535))
	    throw new Exception("Invalid AS number "+iAS);
	ases.add(new Integer(iAS));
    }

    // -----[ get ]--------------------------------------------------
    /**
     * Return the AS-Path segment element at the given position.
     */
    public int get(int iIndex) throws Exception
    {
	if ((iIndex < 0) || (iIndex >= ases.size()))
	    throw new Exception("Invalid AS-Path segment index "+iIndex);
	return ((Integer) ases.get(iIndex)).intValue();
    }

    // -----[ getType ]----------------------------------------------
    /**
     * Return the AS-Path segment's type.
     */
    public int getType()
    {
	return iType;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the AS-Path segment to a String.
     */
    public String toString()
    {
	String s="";

	if (iType == AS_PATH_SEGMENT_SET) {
	    s+= "{";
	}
	for (int iIndex= 0; iIndex < ases.size(); iIndex++) {
	    s+= " ";
	    s+= ases.get(iIndex);
	}
	if (iType == AS_PATH_SEGMENT_SET) {
	    s+= " }";
	}

	return s;
    }

}
