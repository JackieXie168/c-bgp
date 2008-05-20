// ==================================================================
// @(#)ProxyObject.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/02/2006
// @lastdate 12/10/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ ProxyObject ]----------------------------------------------
public class ProxyObject extends Object {

    // -----[ objectId ]---------------------------------------------
    /**
     * This is a replacement for Object's hashCode() method that ensures
     * the returned hashcode is unique.
     */ 
    @SuppressWarnings("unused")
	private long objectId;
	
    // -----[ ProxyObject ]------------------------------------------
    protected ProxyObject(CBGP cbgp) {
    }
	
    // -----[ getCBGP ]----------------------------------------------
    public native CBGP getCBGP();
	
    // -----[ finalize ]---------------------------------------------
    protected void finalize() {
    	_jni_unregister();
    }
	
    // -----[ _jni_unregister ]--------------------------------------
    private native void _jni_unregister();

}
