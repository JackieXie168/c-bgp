// ==================================================================
// @(#)ProxyObject.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/02/2006
// @lastdate 04/06/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ ProxyObject ]---------------------------------------------
public class ProxyObject extends Object
{

    protected CBGP cbgp;
    protected int hash_code;
	
    // -----[ ProxyObject ]-----------------------------------------
    protected ProxyObject(CBGP cbgp)
    {
	this.cbgp= cbgp;
    }
	
    // -----[ getCBGP ]---------------------------------------------
    public CBGP getCBGP()
    {
	return cbgp;
    }
	
    // -----[ finalize ]--------------------------------------------
    protected void finalize()
    {
	_jni_unregister();
    }
	
    // -----[ _jni_unregister ]-----------------------------------------
    private native void _jni_unregister();

    // -----[ hashCode ]------------------------------------------------
    /**
     * This is a replacement for Object's hashCode() method that ensures
     * the returned hashcode is unique. We base it on the memory address
     * of the proxied C object.
     */ 
    public int hashCode()
    {
	return hash_code;
    }
	
}
