// ==================================================================
// @(#)Subnet.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/04/2006
// $Id: Subnet.java,v 1.3 2009-08-31 09:44:52 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.net;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.IPPrefix;

// -----[ Subnet ]--------------------------------------------------
public class Subnet extends Element {
	
	protected IPPrefix prefix;
	
	// -----[ Subnet ]----------------------------------------------
	protected Subnet(CBGP cbgp, IPPrefix prefix) {
		super(cbgp);
		this.prefix= prefix;
	}
	
	// -----[ getId ]-----------------------------------------------
	@Override
	public String getId() {
		return prefix.toString();
	}
	
	// -----[ getPrefix ]-------------------------------------------
	public IPPrefix getPrefix() {
		return prefix;
	}
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return prefix.toString();
	}

}
