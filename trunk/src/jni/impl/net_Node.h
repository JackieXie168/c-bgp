// ==================================================================
// @(#)net_Node.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 19/04/2006
// @lastdate 21/04/2006
// ==================================================================

#ifndef __JNI_NET_NODE_H__
#define __JNI_NET_NODE_H__

#include <jni_md.h>
#include <jni.h>

#include <net/Node.h>
#include <jni/headers/be_ac_ucl_ingi_cbgp_net_Node.h>

// -----[ cbgp_jni_new_net_Node ]------------------------------------
jobject cbgp_jni_new_net_Node(JNIEnv * jEnv, jobject joLink,
			      SNetNode * pNode);

#endif
