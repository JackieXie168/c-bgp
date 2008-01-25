// ==================================================================
// @(#)IPTrace.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 18/02/2004
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.Vector;
import be.ac.ucl.ingi.cbgp.LinkMetrics;
import be.ac.ucl.ingi.cbgp.exceptions.UnknownMetricException;

// -----[ IPTrace ]--------------------------------------------------
/**
 * This class represents the result of an IP record-route or IP
 * traceroute.
 */
public class IPTrace
{

    // -----[ error code constants ]---------------------------------
    public static final int IP_TRACE_SUCCESS           = 0;
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
    protected IPAddress src;
    protected IPAddress dst;
    protected int iStatus;
    protected LinkMetrics metrics;
    protected Vector<IPAddress> hops;
    protected Vector<LinkMetrics> hopsMetrics;

    // -----[ IPTrace ]----------------------------------------------
    /**
     * IPTrace's constructor.
     */
    public IPTrace(IPAddress src, IPAddress dst, int iStatus,
		   LinkMetrics metrics)
    {
	this.src= src;
	this.dst= dst;
	this.iStatus= iStatus;
	this.metrics= metrics;
	this.hops= new Vector<IPAddress>();
	this.hopsMetrics= null;
    }

    // -----[ append ]-----------------------------------------------
    /**
     *
     */
    public void append(IPAddress hop)
    {
    	hops.add(hop);
    }

    // -----[ appendWithInfo ]---------------------------------------
    /**
     *
     */
    public void appendWithInfo(IPAddress hop, LinkMetrics metrics,
			       int iStatus)
    {
    	hops.add(hop);

	// Add hop metrics
	if (hopsMetrics == null)
	    hopsMetrics= new Vector<LinkMetrics>();
	metrics.setMetric("status", new Integer(iStatus));
	hopsMetrics.add(metrics);
    }

    // -----[ getHop ]-----------------------------------------------
    /**
     * Returns the IP address at the given position in the trace.
     */
    public IPAddress getHop(int iIndex)
    {
    	return (IPAddress) hops.get(iIndex);
    }
    
    // -----[ getHopCount ]-----------------------------------------
    /**
     * Returns the number of hops in the trace.
     */
    public int getHopCount()
    {
    	return hops.size();
    }

    // -----[ getStatus ]--------------------------------------------
    /**
     * Returns the trace's status.
     */
    public int getStatus()
    {
	return iStatus;
    }

    // -----[ getCapacity ]------------------------------------------
    /**
     * Returns the trace's capacity.
     */
    public int getCapacity() throws UnknownMetricException
    {
	if (metrics == null)
	    throw new UnknownMetricException();
	return metrics.getCapacity();
    }

    // -----[ getDelay ]---------------------------------------------
    /**
     * Returns the trace's delay.
     */
    public int getDelay() throws UnknownMetricException
    {
	if (metrics == null)
	    throw new UnknownMetricException();
	return metrics.getDelay();
    }

    // -----[ getWeight ]--------------------------------------------
    /**
     * Returns the trace's weight.
     */
    public int getWeight() throws UnknownMetricException
    {
	if (metrics == null)
	    throw new UnknownMetricException();
	return metrics.getWeight();
    }

    // -----[ statusToString ]---------------------------------------
    /**
     *
     */
    public static String statusToString(int iStatus)
    {
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
    public String toString()
    {
	String s= "";
	IPAddress addr;

	s+= src;
	s+= "\t";
	s+= dst;
	s+= "\t";
	s+= statusToString(iStatus);
	s+= "\t";
	for (int iIndex= 0; iIndex < hops.size(); iIndex++) {
	    if (iIndex > 0) {
		s+= " ";
	    }
	    addr= hops.get(iIndex);
	    if (addr != null)
		s+= addr;
	    else
		s+= "*";
	}

	// Show available metrics
	if (metrics != null) {
	    try {
		if (metrics.hasMetric(LinkMetrics.DELAY))
		    s+= "\tdelay:" + metrics.getDelay();
		if (metrics.hasMetric(LinkMetrics.WEIGHT))
		    s+= "\tweight:" + metrics.getWeight();
		if (metrics.hasMetric(LinkMetrics.CAPACITY))
		    s+= "\tcapacity:" + metrics.getCapacity();
	    } catch (UnknownMetricException e) {
	    }
	}

	return s;
    }

}
