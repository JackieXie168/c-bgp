// ==================================================================
// @(#)net_IGPDomain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/04/2006
// @lastdate 21/04/2006
// ==================================================================

#ifndef __JNI_NET_IGPDOMAIN_H__
#define __JNI_NET_IGPDOMAIN_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_net_IGPDomain.h>

#include <net/igp_domain.h>

// -----[ cbgp_jni_new_net_IGPDomain ]-------------------------------
jobject cbgp_jni_new_net_IGPDomain(JNIEnv * jEnv, jobject joCBGP,
				   SIGPDomain * pDomain);

#endif /* __JNI_NET_IGPDOMAIN_H__ */
