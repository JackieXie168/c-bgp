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
