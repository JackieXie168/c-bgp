// ==================================================================
// @(#)IPTrace.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 18/02/2004
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.Collection;
import java.util.Vector;

import be.ac.ucl.ingi.cbgp.net.Node;

// -----[ IPTrace ]--------------------------------------------------
/**
 * This class represents the result of an IP record-route or IP
 * traceroute.
 */
public class IPTrace {

    // -----[ error code constants ]---------------------------------
    public static final int IP_TRACE_SUCCESS           =  0;
    public static final int IP_TRACE_UNKNOWN           = -1;
    public static final int IP_TRACE_TOO_LONG          = -2;
    public static final int IP_TRACE_UNREACH           = -3;
    public static final int IP_TRACE_DOWN              = -4;
    public static final int IP_TRACE_TUNNEL_UNREACH    = -5;
    public static final int IP_TRACE_TUNNEL_BROKEN     = -6;
    public static final int IP_TRACE_ICMP_TIME_EXP     = -7;
    public static final int IP_TRACE_ICMP_HOST_UNREACH = -8;
    public static final int IP_TRACE_ICMP_NET_UNREACH  = -9;

    // -----[ protected attributes ]---------------------------------
    protected Node src;
    protected IPAddress dst;
    protected int status;
    protected Vector<IPTraceElement> elements;

    // -----[ IPTrace ]----------------------------------------------
    /**
     * IPTrace's constructor.
     */
    private IPTrace(Node src, IPAddress dst, int status) {
    	this.src= src;
    	this.dst= dst;
    	this.status= status;
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

    // -----[ appendWithInfo ]---------------------------------------
    /**
     * Add an element to the trace. This methid is privately used by
     * the JNI.
     */
    private void appendWithInfo(IPTraceElement element, LinkMetrics metrics,
			       int iStatus) {
    	elements.add(element);

    	// Add hop metrics
    	/*
    	if (hopsMetrics == null)
    		hopsMetrics= new Vector<LinkMetrics>();
    	metrics.setMetric("status", new Integer(iStatus));
    	hopsMetrics.add(metrics);*/
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
    public static String statusToString(int iStatus) {
    	switch (iStatus) {
    	case IP_TRACE_SUCCESS:
    		return "SUCCESS";
    	case IP_TRACE_TOO_LONG:
    		return "TOO_LONG";
    	case IP_TRACE_UNREACH:
    		return "UNREACH";
    	case IP_TRACE_DOWN:
    		return "DOWN";
    	case IP_TRACE_TUNNEL_UNREACH:
    		return "TUNNEL_UNREACH";
    	case IP_TRACE_TUNNEL_BROKEN:
    		return "TUNNEL_BROKEN";
    	case IP_TRACE_ICMP_TIME_EXP:
    		return "ICMP error (time-expired)";
    	case IP_TRACE_ICMP_HOST_UNREACH:
    		return "ICMP error (host-unreachable)";
    	case IP_TRACE_ICMP_NET_UNREACH:
    		return "ICMP error (network-unreachable)";
    	default:
    		return "UNKNOWN";
    	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the trace to a String.
     */
    public String toString() {
    	String s= src.getAddress().toString();
    	s+= "\t";
    	s+= dst;
    	s+= "\t";
    	s+= statusToString(status);
    	s+= "\t";
    	for (IPTraceElement el: elements) {
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
