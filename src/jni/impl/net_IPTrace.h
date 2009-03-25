// ==================================================================
// @(#)net_IPTrace.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/09/2007
// $Id: net_IPTrace.h,v 1.2 2009-03-25 07:51:59 bqu Exp $
// ==================================================================

#ifndef __JNI_NET_IPTRACE_H__
#define __JNI_NET_IPTRACE_H__

#include <jni_md.h>
#include <jni.h>

#include <net/prefix.h>
#include <net/ip_trace.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cbgp_jni_new_IPTrace ]-----------------------------------
  jobject cbgp_jni_new_IPTrace(JNIEnv * env, ip_trace_t * trace);
  // -----[ cbgp_jni_new_IPTraces ]------------------------------------
  jobjectArray cbgp_jni_new_IPTraces(JNIEnv * env,
				     ip_trace_t ** traces,
				     unsigned int num_traces);
  
#ifdef __cplusplus
}
#endif

#endif /* __JNI_NET_IPTRACE_H__ */
