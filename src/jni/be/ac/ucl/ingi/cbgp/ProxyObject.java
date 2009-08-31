// ==================================================================
// @(#)ProxyObject.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/02/2006
// $Id: ProxyObject.java,v 1.7 2009-08-31 09:40:35 bqu Exp $
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
