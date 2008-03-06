// ==================================================================
// @(#)Subnet.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/04/2006
// @lastdate 24/04/2006
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
