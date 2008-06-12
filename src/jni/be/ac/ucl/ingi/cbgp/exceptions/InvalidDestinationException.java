package be.ac.ucl.ingi.cbgp.exceptions;

@SuppressWarnings("serial")
public class InvalidDestinationException extends CBGPException {

	//	 -----[ InvalidDestinationException ]-----------------------
	protected InvalidDestinationException(String dest) {
		super("The destination \""+dest+"\" is invalid.");
	}
	
}
