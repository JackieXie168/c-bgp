// ==================================================================
// @(#)bgp_Domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/04/2006
// @lastdate 21/04/2006
// ==================================================================

#ifndef __JNI_BGP_DOMAIN_H__
#define __JNI_BGP_DOMAIN_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Domain.h>

#include <bgp/domain.h>

// -----[ cbgp_jni_new_bgp_Domain ]------------------------------------
jobject cbgp_jni_new_bgp_Domain(JNIEnv * jEnv, jobject joCBGP,
				SBGPDomain * pDomain);

#endif /* __JNI_BGP_DOMAIN_H__ */
