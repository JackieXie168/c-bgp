// ==================================================================
// @(#)IPTrace.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: IPTrace.java,v 1.9 2009-08-31 09:40:35 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.Collection;
import java.util.Vector;

import be.ac.ucl.ingi.cbgp.net.Element;

// -----[ IPTrace ]--------------------------------------------------
/**
 * This class represents the result of an IP record-route or IP
 * traceroute.
 */
public class IPTrace {

    // -----[ error code constants ]---------------------------------
    public static final int IP_TRACE_SUCCESS            =  0;
    // Errors on the local node
    public static final int IP_TRACE_NET_UNREACH        = -100;
    public static final int IP_TRACE_HOST_UNREACH       = -101;
    public static final int IP_TRACE_PROTO_UNREACH      = -102;
    public static final int IP_TRACE_TIME_EXCEEDED      = -103;
    public static final int IP_TRACE_LINK_DOWN          = -104;
    // Errors reported by ICMP
    public static final int IP_TRACE_ICMP_NET_UNREACH   = -601;
    public static final int IP_TRACE_ICMP_HOST_UNREACH  = -602;
    public static final int IP_TRACE_ICMP_PROTO_UNREACH = -603;
    public static final int IP_TRACE_ICMP_TIME_EXCEEDED = -604;
    public static final int IP_TRACE_NO_REPLY           = -606;

    // -----[ protected attributes ]---------------------------------
    protected final int status;
    protected final long delay;
    protected final long capacity;
    protected final Vector<IPTraceElement> elements;

    // -----[ IPTrace ]----------------------------------------------
    /**
     * IPTrace's constructor.
     */
    private IPTrace(int status, long delay, long capacity) {
    	this.status= status;
    	this.delay= delay;
    	this.capacity= capacity;
    	elements= new Vector<IPTraceElement>();
    }

    // -----[ append ]-----------------------------------------------
    /**
     * Add an element to the trace. This method is privately used by
     * the JNI.
     */
    @SuppressWarnings("unused")
	private void append(IPTraceElement element) {
    	elements.add(element);
    }

    // -----[ getElementAt ]----------------------------------------
    /**
     * Returns the IP trace element at the given index.
     */
    public IPTraceElement getElementAt(int index) {
    	return elements.get(index);
    }
    
    // -----[ getElementsCount ]------------------------------------
    /**
     * Returns the number of hops in the trace.
     */
    public int getElementsCount() {
    	return elements.size();
    }
    
    // -----[ getElements ]------------------------------------------
    public Collection<IPTraceElement> getElements() {
    	return elements;
    }

    // -----[ getStatus ]--------------------------------------------
    /**
     * Returns the trace's status.
     */
    public int getStatus() {
    	return status;
    }

    // -----[ getCapacity ]------------------------------------------
    /**
     * Returns the trace's capacity.
     */
    /*public int getCapacity() throws UnknownMetricException {
    	if (metrics == null)
    		throw new UnknownMetricException();
    	return metrics.getCapacity();
    }*/

    // -----[ getDelay ]---------------------------------------------
    /**
     * Returns the trace's delay.
     */
    /*public int getDelay() throws UnknownMetricException {
    	if (metrics == null)
    		throw new UnknownMetricException();
    	return metrics.getDelay();
    }*/

    // -----[ getWeight ]--------------------------------------------
    /**
     * Returns the trace's weight.
     */
    /*public int getWeight() throws UnknownMetricException {
    	if (metrics == null)
    		throw new UnknownMetricException();
    	return metrics.getWeight();
    }*/

    // -----[ statusToString ]---------------------------------------
    /**
     *
     */
    public static String statusToString(int status) {
    	//CBGPStatic.getErrorMsg(status);
    	switch (status) {
    	case IP_TRACE_SUCCESS:
	    return "SUCCESS";
	    /*case IP_TRACE_TOO_LONG:
	      return "TOO_LONG";*/
    	case IP_TRACE_NET_UNREACH:
	    return "Network unreachable";
    	case IP_TRACE_HOST_UNREACH:
	    return "Host unreachable";
    	case IP_TRACE_PROTO_UNREACH:
	    return "Protocol unreachable";
    	case IP_TRACE_TIME_EXCEEDED:
	    return "Time exceeded";
    	case IP_TRACE_LINK_DOWN:
	    return "Link down";
	    /*case IP_TRACE_TUNNEL_UNREACH:
	      return "TUNNEL_UNREACH";
	      case IP_TRACE_TUNNEL_BROKEN:
	      return "TUNNEL_BROKEN";*/
    	case IP_TRACE_ICMP_NET_UNREACH:
	    return "ICMP error (network-unreachable)";
    	case IP_TRACE_ICMP_HOST_UNREACH:
	    return "ICMP error (host-unreachable)";
    	case IP_TRACE_ICMP_PROTO_UNREACH:
	    return "ICMP error (protocol-unreachable)";
    	case IP_TRACE_ICMP_TIME_EXCEEDED:
	    return "ICMP error (time-expired)";
    	default:
	    return "UNKNOWN";
    	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the trace to a String.
     */
    public String toString() {
    	String s= "";
    	for (IPTraceElement el: elements) {
    		Element net_el= el.getElement();
    		if (net_el != null)
    			s+= net_el.toString();
    		else
    			s+= el.toString();
    		s+= " ";
    	}

    	// Show available metrics
    	/*if (metrics != null) {
    		try {
    			if (metrics.hasMetric(LinkMetrics.DELAY))
    				s+= "\tdelay:" + metrics.getDelay();
    			if (metrics.hasMetric(LinkMetrics.WEIGHT))
    				s+= "\tweight:" + metrics.getWeight();
    			if (metrics.hasMetric(LinkMetrics.CAPACITY))
    				s+= "\tcapacity:" + metrics.getCapacity();
    		} catch (UnknownMetricException e) { }
    	}*/

    	return s;
    }

}
