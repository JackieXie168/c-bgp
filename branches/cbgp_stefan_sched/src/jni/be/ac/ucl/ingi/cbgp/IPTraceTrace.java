// ==================================================================
// @(#)IPTraceTrace.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id$
// ==================================================================

package be.ac.ucl.ingi.cbgp;

/**
 * This IP trace element models a Subnet.
 * 
 * @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
 */
public class IPTraceTrace extends IPTraceElement {
	
	protected final IPTrace trace;
	
	// -----[ constructor ]-----------------------------------------
	private IPTraceTrace(IPTrace trace) {
		super(null);
		this.trace= trace;
	}
	
	// -----[ getTrace ]--------------------------------------------
	public IPTrace getTrace() {
		return trace;
	}
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return "["+trace.toString()+"]";
	}

}
