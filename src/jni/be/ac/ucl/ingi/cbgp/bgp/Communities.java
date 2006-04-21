// ==================================================================
// @(#)Communities.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/03/2006
// @lastdate 20/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Vector;

// -----[ Communities ]----------------------------------------------
/**
 * This class represents an set of Communities.
 */
public class Communities
{
    
    // -----[ protected attributes ]---------------------------------
    protected Vector communities= null;

    // -----[ Communities ]------------------------------------------
    /**
     * Communities's constructor.
     */
    public Communities()
    {
	this.communities= new Vector();
    }

    // -----[ append ]-----------------------------------------------
    /**
     * Append a new Community.
     */
    public void append(int community)
    {
    	communities.add(new Integer(community));
    }

    // -----[ getCommunityCount ]-----------------------------------
    public int getCommunityCount()
    {
    	return communities.size();
    }
    
    // -----[ getCommunity ]----------------------------------------
    /**
     * Return the Community at the given position.
     */
    public int getCommunity(int iIndex)
    {
    	return ((Integer) communities.get(iIndex)).intValue();
    }
    
    // -----[ communityToString ]------------------------------------
    /**
     * Convert a community to its String representation.
     */
    public static String communityToString(int community)
    {
	long lCommunity= (community >= 0)
	    ?community
	    :(0x100000000L +community);
	String s= "";
	s+= (lCommunity >> 16);
	s+= ":";
	s+= (lCommunity & 0xffff);
	return s;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the Communities to a String.
     */
    public String toString()
    {
	String s= "";

	for (int iIndex= communities.size(); iIndex > 0; iIndex--) {
	    if (iIndex < communities.size()) {
		s+= " ";
	    }
	    s+= communityToString(getCommunity(iIndex-1));
	}
	return s;
    }

}
