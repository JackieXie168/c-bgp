// ==================================================================
// @(#)IGPDomain.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 17/03/2006
// @lastdate 17/03/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp.net; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.*;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ IGPDomain ]------------------------------------------------
/**
 * This class represents an IGP domain.
 */
public class IGPDomain extends ProxyObject
{

    // -----[ IGP domain types ]-------------------------------------
    public static final int DOMAIN_IGP = 0;
    public static final int DOMAIN_OSPF= 1;

    // -----[ protected attributes ]---------------------------------
    protected int id;
    protected int type;

    // -----[ IGPDomain ]--------------------------------------------
    /**
     * IGPDomain's constructor.
     */
    public IGPDomain(CBGP cbgp, int id, int type) {
    	super(cbgp);
    	this.id= id;
    	this.type= type;
    }

    // -----[ getID ]------------------------------------------------
    /**
     * Returns the domain's ID.
     */
    public int getID() {
    	return id;
    }

    // -----[ getType ]----------------------------------------------
    /**
     * Returns the domain's type.
     */
    public int getType() {
    	return type;
    }
    
    // -----[ addNode ]---------------------------------------------
    public native synchronized Node addNode(String sAddr)
    	throws CBGPException;
    
    // -----[ getNodes ]--------------------------------------------
    public native synchronized Vector<Node> getNodes()
		throws CBGPException;
    
    // -----[ compute ]---------------------------------------------
    public native synchronized void compute()
		throws CBGPException;

    // -----[ typeToString ]-----------------------------------------
    /**
     * Convert an IGP domain type to a String.
     */
    public static String typeToString(int type) {
    	String s;
    	switch (type) {
    	case DOMAIN_IGP:
    		s= "IGP"; break;
    	case DOMAIN_OSPF:
    		s= "OSPF"; break;
    	default:
    		s= "?";
    	}
    	return s;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this IGPDomain to a String.
     */
    public String toString() {
    	return "Domain "+id+" (type:"+typeToString(type)+")";
    }

}
