// ==================================================================
// @(#)TestTraceRoute.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestTraceRoute.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.IPTrace;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Node;

public class TestTraceRoute extends TestCommon {
	
	protected CBGP cbgp;
	protected IGPDomain domain;
	protected Node node1, node2, node3;
	
	@Before
	public void setUp() throws CBGPException {
		cbgp= new CBGP();
		cbgp.init(null);
		domain= cbgp.netAddDomain(1);
		node1= domain.addNode("1.0.0.1");
		node2= domain.addNode("1.0.0.2");
		node3= domain.addNode("1.0.0.3");
		node1.addLTLLink(node2, false).setWeight(1);
		node2.addLTLLink(node1, false).setWeight(1);
		node2.addLTLLink(node3, false).setWeight(2);
		node3.addLTLLink(node2, false).setWeight(2);
		domain.compute();
	}
	
	@After
	public void tearDown() throws CBGPException {
		cbgp.destroy();
	}
	
	@Test
	public void testBasic() throws CBGPException {
	    /* A trace's length is >= 1 as it always includes the source node */
	    IPTrace trace= node1.traceRoute("1.0.0.2");
	    assertNotNull(trace);
	    assertEquals(IPTrace.IP_TRACE_SUCCESS, trace.getStatus());
	    assertEquals((int) 2, trace.getElementsCount());

	    trace= node1.traceRoute("1.0.0.3");
	    assertNotNull(trace);
	    assertEquals(IPTrace.IP_TRACE_SUCCESS, trace.getStatus());
	    assertEquals((int) 3, trace.getElementsCount());

	    trace= node1.traceRoute("1.0.0.4"); // node does not exist
	    assertEquals(IPTrace.IP_TRACE_HOST_UNREACH, trace.getStatus());
	    assertEquals((int) 2, trace.getElementsCount());
	}
	
}
