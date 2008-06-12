package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

public class TestSession {

	@Test
	public void testNormal() {
		CBGP cbgp= new CBGP();
		try {
			cbgp.init(null);
			assertTrue(cbgp.getVersion().startsWith("1.5."));
			cbgp.destroy();
		} catch (CBGPException e) {
			fail();
		} 
	}

	@Test
	public void testDuplicateInit() {
		CBGP cbgp= new CBGP();
		try { cbgp.init(null);
		} catch (CBGPException e) { fail(); }
		try {
			cbgp.init(null);
			fail();
		} catch (CBGPException e) {	}
		try { cbgp.destroy();
		} catch (CBGPException e) {	fail(); }
	}
	
	@Test
	public void testMultiple()	{
		CBGP cbgp= new CBGP();
		try {
			cbgp.init(null);
			cbgp.destroy();
			cbgp.init(null);
			cbgp.destroy();
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@Test(expected=NullPointerException.class)
	public void TestNull() {
		CBGP cbgp= null;
		try {
			cbgp.init(null);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
	
}
