// ==================================================================
// @(#)net_Node.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/04/2006
// $Id: net_Node.h,v 1.5 2008-04-10 11:27:00 bqu Exp $
// ==================================================================

#ifndef __JNI_NET_NODE_H__
#define __JNI_NET_NODE_H__

#include <jni_md.h>
#include <jni.h>

#include <net/node.h>
#include <jni/headers/be_ac_ucl_ingi_cbgp_net_Node.h>

// -----[ cbgp_jni_new_net_Node ]------------------------------------
jobject cbgp_jni_new_net_Node(JNIEnv * jEnv, jobject joCBGP,
			      net_node_t * node);

#endif
