// ==================================================================
// @(#)InvalidDestinationException.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: InvalidDestinationException.java,v 1.2 2009-08-31 09:44:07 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.exceptions;

@SuppressWarnings("serial")
public class InvalidDestinationException extends CBGPException {

	//	 -----[ InvalidDestinationException ]-----------------------
	protected InvalidDestinationException(String dest) {
		super("The destination \""+dest+"\" is invalid.");
	}
	
}
