// ==================================================================
// @(#)TestScript.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestScript.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPScriptException;

public class TestScript {

	CBGP cbgp;
	
	@Before
	public void setUp() {
		cbgp= new CBGP();
		try {
			cbgp.init(null);
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@After
	public void tearDown() {
		try {
			cbgp.destroy();
			cbgp= null;
		} catch (CBGPException e) {
			fail();
		}
	}
	
	private void writeScript(String filename, String script)
		throws IOException
	{
        BufferedWriter out= new BufferedWriter(new FileWriter(filename));
        out.write(script);
        out.close();
	}

	@Test
	public void testSingle() throws CBGPException {
		cbgp.runCmd("net add node 1.0.0.1");
	}
	
	@Test
	public void testBasic() throws IOException {
		final String filename= "/tmp/cbgp-jni-junit-script.cli";
        writeScript(filename,
        		"net add node 1.0.0.1\n"+
        		"net add node 1.0.0.1\n");
        try {
        	cbgp.runScript(filename);
        } catch (CBGPScriptException e) {
        	assertEquals(2, e.getLine());
        	// assertEquals(filename, e.getFileName());
        }
	}
	
	@Test
	public void testInclude() throws IOException {
		final String filename1= "/tmp/cbgp-jni-junit-script1.cli";
		final String filename2= "/tmp/cbgp-jni-junit-script2.cli";
        writeScript(filename1,
        		"net add node 1.0.0.1\n"+
        		"net add node 1.0.0.1\n");
        writeScript(filename2,
        		"include \""+filename1+"\"\n");
        try {
        	cbgp.runScript(filename2);
        } catch (CBGPScriptException e) {
        	assertEquals(1, e.getLine());
        	//assertEquals(filename1, e.getFileName());
        }
	}
	
}
