// ==================================================================
// @(#)IPTraceElement.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: IPTraceElement.java,v 1.3 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp;

import be.ac.ucl.ingi.cbgp.net.Element;

/**
 * This class represents an element of an IP trace. These elements
 * can be Nodes or Subnets.
 * 
 * @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
 */
public class IPTraceElement {

	protected int hop_count;
	protected final Element element;
	
	// -----[ constructor ]-----------------------------------------
	protected IPTraceElement(Element element) {
		this.element= element;
	}
	
	// -----[ getElement ]------------------------------------------
	public Element getElement() {
		return this.element;
	}
	
	// -----[ getHopCount ]-----------------------------------------
	public int getHopCount() {
		return hop_count;
	}
	
}
