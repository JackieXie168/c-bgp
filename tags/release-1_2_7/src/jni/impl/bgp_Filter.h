// ==================================================================
// @(#)bgp_Filter.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/04/2006
// @lastdate 25/04/2006
// ==================================================================

#ifndef __JNI_BGP_FILTER_H__
#define __JNI_BGP_FILTER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Filter.h>
#include <bgp/filter_t.h>

// -----[ cbgp_jni_new_bgp_Filter ]------------------------------------
jobject cbgp_jni_new_bgp_Filter(JNIEnv * jEnv, jobject joCBGP,
				SFilter * pFilter);

#endif /* __JNI_BGP_FILTER_H__ */
