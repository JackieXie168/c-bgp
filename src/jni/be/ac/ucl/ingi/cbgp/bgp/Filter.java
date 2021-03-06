// ==================================================================
// @(#)Filter.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/04/2006
// $Id: Filter.java,v 1.4 2009-08-31 09:42:43 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp;

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.ProxyObject;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

public class Filter extends ProxyObject {

	// -----[ constructor ]-----------------------------------------
	protected Filter(CBGP cbgp)	{
		super(cbgp);
	}
	
	// -----[ getRules ]--------------------------------------------
	public native Vector<FilterRule> getRules()
		throws CBGPException;
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return "Filter";
	}
	
}
