// ==================================================================
// @(#)bgp_FilterRule.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/05/2008
// $Id: bgp_FilterRule.h,v 1.1 2008-06-11 15:21:52 bqu Exp $
// ==================================================================

#ifndef __JNI_BGP_FILTERRULE_H__
#define __JNI_BGP_FILTERRULE_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_FilterRule.h>
#include <bgp/filter_t.h>

// -----[ cbgp_jni_new_bgp_FilterRule ]------------------------------
jobject cbgp_jni_new_bgp_FilterRule(JNIEnv * jEnv, jobject joCBGP,
				    bgp_ft_rule_t * rule);

#endif /* __JNI_BGP_FILTERRULE_H__ */
