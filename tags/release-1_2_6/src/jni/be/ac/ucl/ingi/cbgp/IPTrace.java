// ==================================================================
// @(#)IPTrace.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 18/02/2004
// @lastdate 19/02/2005
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.Vector;

// -----[ IPTrace ]--------------------------------------------------
/**
 * This class represents the result of an IP record-route.
 */
public class IPTrace
{

    // -----[ constants ]--------------------------------------------
    public static final int IP_TRACE_SUCCESS       = 0;
    public static final int IP_TRACE_TOO_LONG      = -1;
    public static final int IP_TRACE_UNREACH       = -2;
    public static final int IP_TRACE_DOWN          = -3;
    public static final int IP_TRACE_TUNNEL_UNREACH= -4;
    public static final int IP_TRACE_TUNNEL_BROKEN = -5;

    // -----[ protected attributes ]---------------------------------
    protected IPAddress src;
    protected IPAddress dst;
    protected int iStatus;
    protected int iDelay;
    protected int iWeight;
    protected Vector hops;

    // -----[ IPTrace ]----------------------------------------------
    /**
     * IPTrace's constructor.
     */
    public IPTrace(IPAddress src, IPAddress dst,
		   int iStatus, int iDelay, int iWeight)
    {
	this.src= src;
	this.dst= dst;
	this.iStatus= iStatus;
	this.iDelay= iDelay;
	this.iWeight= iWeight;
	this.hops= new Vector();
    }

    // -----[ append ]-----------------------------------------------
    /**
     *
     */
    public void append(IPAddress hop)
    {
    	hops.add(hop);
    }

    // -----[ getDelay ]---------------------------------------------
    /**
     * Returns the trace's delay.
     */
    public int getDelay()
    {
	return iDelay;
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

    // -----[ getWeight ]--------------------------------------------
    /**
     * Returns the trace's weight.
     */
    public int getWeight()
    {
	return iWeight;
    }

    // -----[ statusToString ]---------------------------------------
    /**
     *
     */
    public static String statusToString(int iStatus)
    {
	switch (iStatus) {
	case IP_TRACE_SUCCESS: return "SUCCESS";
	case IP_TRACE_TOO_LONG: return "TOO_LONG";
	case IP_TRACE_UNREACH: return "UNREACH";
	case IP_TRACE_DOWN: return "DOWN";
	case IP_TRACE_TUNNEL_UNREACH: return "TUNNEL_UNREACH";
	case IP_TRACE_TUNNEL_BROKEN: return "TUNNEL_BROKEN";
	default:
	    return "?";
	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the trace to a String.
     */
    public String toString()
    {
	String s= "";

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
	    s+= hops.get(iIndex);
	}
	s+= "\t";
	s+= iDelay;
	s+= "\t";
	s+= iWeight;

	return s;
    }

}
