package be.ac.ucl.ingi.cbgp.testing;

import junit.framework.JUnit4TestAdapter;
import junit.framework.Test;

import org.junit.runner.JUnitCore;
import org.junit.runner.Result;
import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

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
	
	public static void main(String[] args) {
		System.out.println("Running JUnit tests (version:"+(new JUnitCore()).getVersion()+")");
		/*TestResult result = new TestResult();
		suite().run(result);*/
		Result result= JUnitCore.runClasses(new Class [] {AllTests.class});
		System.out.println(result.toString());
	}
	
	public static Test suite() {
		return new JUnit4TestAdapter(AllTests.class);
	}
		
}
