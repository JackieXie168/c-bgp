package be.ac.ucl.ingi.cbgp.testing;

import org.junit.Rule;
import org.junit.runner.Description;
import org.junit.rules.TestRule;
import org.junit.rules.TestWatcher;

public class TestCommon
{

    @Rule
    public TestRule watcher = new TestWatcher() {
	    protected void starting(Description description) {
		System.out.println("*** Starting test : " + description.getTestClass().getSimpleName() + '.' + description.getMethodName() + " ***");
	    }
	};

}
