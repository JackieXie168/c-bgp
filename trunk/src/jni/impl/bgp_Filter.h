// ==================================================================
// @(#)bgp_Filter.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/04/2006
// $Id: bgp_Filter.h,v 1.4 2009-03-25 07:49:45 bqu Exp $
// ==================================================================

#ifndef __JNI_BGP_FILTER_H__
#define __JNI_BGP_FILTER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Filter.h>
#include <bgp/filter/types.h>

// -----[ cbgp_jni_new_bgp_Filter ]------------------------------------
jobject cbgp_jni_new_bgp_Filter(JNIEnv * jEnv, jobject joCBGP,
				bgp_filter_t * filter);

#endif /* __JNI_BGP_FILTER_H__ */
