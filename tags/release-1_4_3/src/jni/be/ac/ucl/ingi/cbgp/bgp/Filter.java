// ==================================================================
// @(#)Filter.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/04/2006
// @lastdate 25/04/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp.bgp;

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.CBGPException;
import be.ac.ucl.ingi.cbgp.ProxyObject;

// -----[ Filter ]--------------------------------------------------
public class Filter extends ProxyObject {

	// -----[ Filter ]----------------------------------------------
	protected Filter(CBGP cbgp)
	{
		super(cbgp);
	}
	
	// -----[ getRules ]--------------------------------------------
	public native Vector getRules()
		throws CBGPException;
	
	// -----[ toString ]--------------------------------------------
	public String toString()
	{
		return "Filter";
	}
	
}
