// ==================================================================
// @(#)CBGPScriptException.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: CBGPScriptException.java,v 1.2 2009-08-31 09:44:07 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.exceptions;

@SuppressWarnings("serial")
public class CBGPScriptException extends CBGPException {

	private int line;
	private String filename;
	
	// -----[ CBGPScriptException ]---------------------------------
	public CBGPScriptException(String msg, String filename, int line) {
		super(msg);
		this.filename= filename;
		this.line= line;
	}
	
	// -----[ getLine ]---------------------------------------------
	public int getLine() {
		return line;
	}

	// -----[ getFileName ]-----------------------------------------
	public String getFileName() {
		return filename;
	}

}
