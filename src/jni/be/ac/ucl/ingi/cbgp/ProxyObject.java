// ==================================================================
// @(#)ProxyObject.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/02/2006
// @lastdate 18/04/2006
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

// -----[ ProxyObject ]---------------------------------------------
public class ProxyObject extends Object
{

	protected CBGP cbgp;
	
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
	
}
