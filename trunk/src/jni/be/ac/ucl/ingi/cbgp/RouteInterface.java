// ==================================================================
// @(#)RouteInterface.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/02/2005
// $Id: RouteInterface.java,v 1.2 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ RouteInterface ]-------------------------------------------
/**
 * This is an interface for routes.
 */
public interface RouteInterface {

    // -----[ isBest ]-----------------------------------------------
    public boolean isBest();

    // -----[ isFeasible ]-------------------------------------------
    public boolean isFeasible();

    // -----[ getNexthop ]-------------------------------------------
    public IPAddress getNexthop();

    // -----[ getPrefix ]--------------------------------------------
    public IPPrefix getPrefix();

}
