// ==================================================================
// @(#)IPTraceNode.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: IPTraceNode.java,v 1.3 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp;

import be.ac.ucl.ingi.cbgp.net.Interface;
import be.ac.ucl.ingi.cbgp.net.Node;

/**
 * This IP trace element models a Node along with its incoming and
 * outgoing interface (i.e. how the node was reached and how it
 * reached the next-hop element.
 * 
 * The incoming and outgoing interface elements are optional.
 * However, without that information, it might be impossible to
 * rebuild the entire trace's path.
 * 
 * Examples:
 * - In the case of traceroute, the incoming interface represents
 *   the interface that was used by the node to reply the ICMP Echo
 *   Request datagram.
 * - In the case of record-route, the incoming and outgoing
 *   interfaces are known and defined, except for the first and last
 *   nodes in the trace.
 * 
 * @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
 */
public class IPTraceNode extends IPTraceElement {
	
	private Interface iif, oif;
	
	// -----[ constructor ]-----------------------------------------
	private IPTraceNode(Node node, Interface iif, Interface oif) {
		super(node);
		this.iif= iif;
		this.oif= oif;
	}

	// -----[ getNode ]---------------------------------------------
	public Node getNode() {
		return (Node) element;
	}
	
	// -----[ getInIface ]------------------------------------------
	public Interface getInIface() {
		return iif;
	}
	
	// -----[ getOutIface ]-----------------------------------------
	public Interface getOutIface() {
		return oif;
	}
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return "Node("+getNode()+")";
	}
	
}
