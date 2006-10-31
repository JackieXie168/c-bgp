// ==================================================================
// @(#)net_Link.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 21/04/2006
// ==================================================================

#ifndef __JNI_NET_LINK_H__
#define __JNI_NET_LINK_H__

#include <jni_md.h>
#include <jni.h>

#include <net/link.h>
#include <jni/headers/be_ac_ucl_ingi_cbgp_net_Link.h>

// -----[ cbgp_jni_new_net_Link ]------------------------------------
jobject cbgp_jni_new_net_Link(JNIEnv * jEnv, jobject joLink,
			      SNetLink * pLink);

#endif
