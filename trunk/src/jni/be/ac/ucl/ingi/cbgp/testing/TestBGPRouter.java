// ==================================================================
// @(#)TestBGPRouter.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestBGPRouter.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.bgp.Router;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;

public class TestBGPRouter extends TestCommon {
	
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
			IGPDomain domain= cbgp.netAddDomain(123);
			domain.addNode("1.0.0.1");
			Router router1= cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
			assertNotNull(router1);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
	
	@Test(expected=CBGPException.class)
	public void testNoNode() throws CBGPException {
		cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
	}
	
	@Test(expected=CBGPException.class)
	public void testDuplicate() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		domain.addNode("1.0.0.1");
		cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
		cbgp.bgpAddRouter("RT2", "1.0.0.1", 123);
	}
	
	@Test
	public void testAddNetwork() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		domain.addNode("1.0.0.1");
		Router router= cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
		router.addNetwork("10.0.1/24");
		router.addNetwork("10.0.2/24");
	}
	
	@Test(expected=CBGPException.class)
	public void testAddNetworkDup() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		domain.addNode("1.0.0.1");
		Router router= cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
		router.addNetwork("10.0.1/24");
		router.addNetwork("10.0.1/24");
	}
	
}
