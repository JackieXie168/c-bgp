// ==================================================================
// @(#)Element.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: Element.java,v 1.3 2009-08-31 09:44:52 bqu Exp $
// ==================================================================
package be.ac.ucl.ingi.cbgp.net;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.ProxyObject;

public abstract class Element extends ProxyObject {
	
	// -----[ constructor ]-----------------------------------------
	protected Element(CBGP cbgp) {
		super(cbgp);
	}
	
	// -----[ getId ]-----------------------------------------------
	public abstract String getId();
	
	// -----[ equals ]----------------------------------------------
	@Override
	public boolean equals(Object obj) {
		if (obj == null)
			return false;
		if (obj.getClass() != this.getClass())
			return false;
		return ((Element) obj).getId().equals(getId());
	}
	
	// -----[ hashCode ]--------------------------------------------
	@Override
	public int hashCode() {
		return getId().hashCode();
	}

}
