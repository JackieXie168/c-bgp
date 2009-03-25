// ==================================================================
// @(#)bgp_FilterRule.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/05/2008
// $Id: bgp_FilterRule.h,v 1.2 2009-03-25 07:49:45 bqu Exp $
// ==================================================================

#ifndef __JNI_BGP_FILTERRULE_H__
#define __JNI_BGP_FILTERRULE_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_FilterRule.h>
#include <bgp/filter/types.h>

// -----[ cbgp_jni_new_bgp_FilterRule ]------------------------------
jobject cbgp_jni_new_bgp_FilterRule(JNIEnv * jEnv, jobject joCBGP,
				    bgp_ft_rule_t * rule);

#endif /* __JNI_BGP_FILTERRULE_H__ */
