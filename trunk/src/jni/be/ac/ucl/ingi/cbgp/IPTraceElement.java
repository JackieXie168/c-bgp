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
	protected Element element;
	
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
