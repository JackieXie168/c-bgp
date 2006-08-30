// ==================================================================
// @(#)Route.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/02/2005
// @lastdate 08/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ Route ]----------------------------------------------------
/**
 * This is a basic implementation of the RouteInterface.
 */
public class Route
{

    // -----[ protected attributes of the route ]--------------------
    protected IPPrefix prefix= null;
    protected IPAddress nexthop= null;
    protected boolean bBest;
    protected boolean bFeasible;

    // -----[ Route ]------------------------------------------------
    public Route(IPPrefix prefix, IPAddress nexthop,
		 boolean bBest, boolean bFeasible)
    {
	this.prefix= prefix;
	this.nexthop= nexthop;
	this.bBest= bBest;
	this.bFeasible= bFeasible;
    }

    // -----[ isBest ]-----------------------------------------------
    public boolean isBest()
    {
	return bBest;
    }

    // -----[ isFeasible ]-------------------------------------------
    public boolean isFeasible()
    {
	return bFeasible;
    }

    // -----[ getNexthop ]-------------------------------------------
    public IPAddress getNexthop()
    {
	return nexthop;
    }

    // -----[ getPrefix ]--------------------------------------------
    public IPPrefix getPrefix()
    {
	return prefix;
    }

}
