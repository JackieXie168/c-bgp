// ==================================================================
// @(#)Link.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/02/2005
// @lastdate 08/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ Link ]-----------------------------------------------------
/**
 * This class is a container for a link.
 */
public class Link
{

    // -----[ public constants ]-------------------------------------

    // -----[ protected attributes ]---------------------------------
    protected IPAddress nexthopIf;
    protected long lWeight;
    protected long lDelay;
    protected boolean bStatus;

    // -----[ Link ]-------------------------------------------------
    /**
     * Link's constructor.
     */
    public Link(IPAddress nexthopIf, long lDelay, long lWeight,
		boolean bStatus)
    {
	/* Attributes */
	this.nexthopIf= nexthopIf;
	this.lDelay= lDelay;
	this.lWeight= lWeight;

	/* Flags */
	this.bStatus= bStatus;
    }

    // -----[ getDelay ]---------------------------------------------
    /**
     * Return the link's propagation delay.
     */
    public long getDelay()
    {
	return lDelay;
    }

    // -----[ getNexthopIf ]-----------------------------------------
    /**
     * Return the link's nexthop interface.
     */
    public IPAddress getNexthopIf()
    {
	return nexthopIf;
    }

    // -----[ getWeight ]--------------------------------------------
    /**
     * Return the link's IGP weight.
     */
    public long getWeight()
    {
	return lWeight;
    }

    // -----[ IsUp ]-------------------------------------------------
    /**
     * Returns the link's status. If the link is up, true is
     * returned. Otherwise, false is returned.
     */
    public boolean isUp()
    {
	return bStatus;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert this link to a String.
     */
    public String toString()
    {
	String s= "";

	/* Attributes */
	s+= nexthopIf;
	s+= "\t";
	s+= lDelay;
	s+= "\t";
	s+= lWeight;
	s+= "\t";

	/* Flags */
	s+= (bStatus?"UP":"DOWN");
	s+= "\t";

	s+= "DIRECT";

	return s;
    }

}
