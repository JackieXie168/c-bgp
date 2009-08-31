// ==================================================================
// @(#)IPTraceSubnet.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: IPTraceSubnet.java,v 1.2 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp;

import be.ac.ucl.ingi.cbgp.net.Subnet;

/**
 * This IP trace element models a Subnet.
 * 
 * @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
 */
public class IPTraceSubnet extends IPTraceElement {
	
	// -----[ constructor ]-----------------------------------------
	private IPTraceSubnet(Subnet subnet) {
		super(subnet);
	}
	
	// -----[ getSubnet ]-------------------------------------------
	public Subnet getSubnet() {
		return (Subnet) element;
	}
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return "Subnet("+getSubnet()+")";
	}

}
