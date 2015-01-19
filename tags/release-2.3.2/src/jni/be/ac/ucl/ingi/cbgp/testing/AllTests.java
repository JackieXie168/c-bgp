// ==================================================================
// @(#)AllTests.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: AllTests.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
// ==================================================================

package be.ac.ucl.ingi.cbgp.testing;

import junit.framework.JUnit4TestAdapter;
import junit.framework.Test;

import org.junit.runner.JUnitCore;
import org.junit.runner.Result;
import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

import org.junit.Rule;
import org.junit.runner.Description;
import org.junit.rules.TestRule;
import org.junit.rules.TestWatcher;

@RunWith(Suite.class)
@SuiteClasses({
	TestSession.class,
	TestScript.class,
	TestNode.class,
	TestSubnet.class,
	TestLink.class,
	TestIGPDomain.class,
	TestBGPDomain.class,
	TestBGPRouter.class,
	TestBGPRoutes.class,
	TestMisc.class,
	TestRecordRoute.class,
	TestTraceRoute.class
})
public class AllTests {

    @Rule
    public TestRule watcher = new TestWatcher() {
	    protected void starting(Description description) {
		System.out.println("Starting test: " + description.getMethodName());
	    }
	};
    
    public static Test suite() {
	return new JUnit4TestAdapter(AllTests.class);
    }
		
}
