package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.*;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.Subnet;

public class TestSubnet {
	
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
	
	@Test
	public void testNormal() {
		try {
			Subnet subnet= cbgp.netAddSubnet("192.168/16", 0);
			assertNotNull(subnet);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}

}
