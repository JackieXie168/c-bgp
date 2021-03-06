// ==================================================================
// @(#)Communities.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/03/2006
// $Id: Communities.java,v 1.3 2009-08-31 09:42:43 bqu Exp $
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
    protected Vector<Integer> communities= null;

    // -----[ Communities ]------------------------------------------
    /**
     * Communities's constructor.
     */
    public Communities() {
    	this.communities= new Vector<Integer>();
    }

    // -----[ append ]-----------------------------------------------
    /**
     * Append a new Community.
     */
    public void append(int community) {
    	communities.add(new Integer(community));
    }

    // -----[ getCommunityCount ]-----------------------------------
    public int getCommunityCount() {
    	return communities.size();
    }
    
    // -----[ getCommunity ]----------------------------------------
    /**
     * Return the Community at the given position.
     */
    public int getCommunity(int iIndex) {
    	return ((Integer) communities.get(iIndex)).intValue();
    }
    
    // -----[ communityToString ]------------------------------------
    /**
     * Convert a community to its String representation.
     */
    public static String communityToString(int community) {
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
    public String toString() {
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
