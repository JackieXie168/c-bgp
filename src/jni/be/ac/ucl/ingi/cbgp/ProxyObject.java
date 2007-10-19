// ==================================================================
// @(#)ProxyObject.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/02/2006
// @lastdate 12/10/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ ProxyObject ]----------------------------------------------
public class ProxyObject extends Object
{

    protected CBGP cbgp;

    // -----[ objectId ]---------------------------------------------
    /**
     * This is a replacement for Object's hashCode() method that ensures
     * the returned hashcode is unique.
     */ 
    @SuppressWarnings("unused")
	private long objectId;
	
    // -----[ ProxyObject ]------------------------------------------
    protected ProxyObject(CBGP cbgp)
    {
    	this.cbgp= cbgp;
    }
	
    // -----[ getCBGP ]----------------------------------------------
    public CBGP getCBGP()
    {
    	return cbgp;
    }
	
    // -----[ finalize ]---------------------------------------------
    protected void finalize()
    {
    	_jni_unregister();
    }
	
    // -----[ _jni_unregister ]--------------------------------------
    private native void _jni_unregister();

}
