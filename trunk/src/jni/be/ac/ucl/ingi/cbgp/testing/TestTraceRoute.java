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

public class TestTraceRoute {
	
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
		node1.addLTLLink(node2, true).setWeight(1);
		node2.addLTLLink(node3, true).setWeight(2);
		domain.compute();
	}
	
	@After
	public void tearDown() throws CBGPException {
		cbgp.destroy();
	}
	
	@Test
	public void testBasic() throws CBGPException {
		IPTrace trace= node1.traceRoute("1.0.0.2");
		assertNotNull(trace);
		assertEquals(IPTrace.IP_TRACE_SUCCESS, trace.getStatus());
		assertEquals((int) 2, trace.getElementsCount());
		trace= node1.recordRoute("1.0.0.3");
		assertNotNull(trace);
		assertEquals(IPTrace.IP_TRACE_SUCCESS, trace.getStatus());
		assertEquals((int) 3, trace.getElementsCount());
	}
	
}
