// ==================================================================
// @(#)RouteInterface.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 08/02/2005
// @lastdate 08/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ RouteInterface ]-------------------------------------------
/**
 * This is an interface for routes.
 */
public interface RouteInterface
{

    // -----[ isBest ]-----------------------------------------------
    public boolean isBest();

    // -----[ isFeasible ]-------------------------------------------
    public boolean isFeasible();

    // -----[ getNexthop ]-------------------------------------------
    public IPAddress getNexthop();

    // -----[ getPrefix ]--------------------------------------------
    public IPPrefix getPrefix();

}
