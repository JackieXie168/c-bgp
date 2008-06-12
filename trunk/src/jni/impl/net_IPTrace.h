// ==================================================================
// @(#)net_IPTrace.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/09/2007
// @lastdate 03/03/2008
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
  jobject cbgp_jni_new_IPTrace(JNIEnv * jEnv,
			       net_node_t * pNode,
			       net_addr_t tDst,
			       ip_trace_t * trace);
  
#ifdef __cplusplus
}
#endif

#endif /* __JNI_NET_IPTRACE_H__ */
