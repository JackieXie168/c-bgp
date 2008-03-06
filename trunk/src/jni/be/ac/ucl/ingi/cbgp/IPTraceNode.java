package be.ac.ucl.ingi.cbgp;

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
	
	private IPAddress iif, oif;
	
	// -----[ constructor ]-----------------------------------------
	private IPTraceNode(Node node, IPAddress iif, IPAddress oif) {
		super(node);
		this.iif= iif;
		this.oif= oif;
	}

	// -----[ getNode ]---------------------------------------------
	public Node getNode() {
		return (Node) element;
	}
	
	// -----[ getInIface ]------------------------------------------
	public IPAddress getInIface() {
		return iif;
	}
	
	// -----[ getOutIface ]-----------------------------------------
	public IPAddress getOutIface() {
		return oif;
	}
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return "Node("+getNode()+")";
	}
	
}
