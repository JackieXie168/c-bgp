// ==================================================================
// @(#)net_Link.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/03/2006
// $Id: net_Link.c,v 1.11 2008-04-07 10:04:59 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni_md.h>
#include <jni.h>
#include <jni/exceptions.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/net_Subnet.h>

#include <net/link.h>

#define CLASS_Link "be/ac/ucl/ingi/cbgp/net/Link"
#define CONSTR_Link "(Lbe/ac/ucl/ingi/cbgp/CBGP;"\
                    "Lbe/ac/ucl/ingi/cbgp/IPPrefix;J)V"

// -----[ cbgp_jni_new_net_Link ]------------------------------------
/**
 * This function creates a new instance of the Link object from a CBGP
 * link.
 */
jobject cbgp_jni_new_net_Link(JNIEnv * jEnv, jobject joCBGP,
			      net_iface_t * pLink)
{
  jobject joLink;
  jobject joPrefix;

  /* Java proxy object already existing ? */
  joLink= jni_proxy_get(jEnv, pLink);
  if (joLink != NULL)
    return joLink;

  /* Convert link attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, net_iface_dst_prefix(pLink));
  if (joPrefix == NULL)
    return NULL;

  /* Create new Link object */
  if ((joLink= cbgp_jni_new(jEnv, CLASS_Link, CONSTR_Link,
			    joCBGP, joPrefix,
			    (jlong) net_iface_get_delay(pLink))) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joLink, pLink);

  return joLink;
}

// -----[ getCapacity ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getCapacity
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getCapacity
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jlong jlCapacity= 0;

  jni_lock(jEnv);

  /* Get interface */
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, -1);

  jlCapacity= net_iface_get_capacity(pIface);

  return_jni_unlock(jEnv, jlCapacity);
}

// -----[ setCapacity ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setCapacity
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setCapacity
  (JNIEnv * jEnv, jobject joIface, jlong jlCapacity)
{
  net_iface_t * pIface;
  int iResult;

  jni_lock(jEnv);

  /* Get interface */
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock2(jEnv);

  iResult= net_iface_set_capacity(pIface, jlCapacity, 0);
  if (iResult != ESUCCESS)
    throw_CBGPException(jEnv, "could not set capacity (%s)",
			network_strerror(iResult));

  jni_unlock(jEnv);
}

// -----[ getLoad ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getLoad
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getLoad
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jlong jlLoad= 0;

  jni_lock(jEnv);

  /* Get interface */
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, -1);

  jlLoad= net_iface_get_load(pIface);

  return_jni_unlock(jEnv, jlLoad);
}

// -----[ getState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getState
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getState
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jboolean jbState;

  jni_lock(jEnv);
  
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jbState= net_iface_is_enabled(pIface)?JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jbState);
}

// -----[ setState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setState
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setState
  (JNIEnv * jEnv, jobject joIface, jboolean bState)
{
  net_iface_t * pIface;

  jni_lock(jEnv);

  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock2(jEnv);
    
  net_iface_set_enabled(pIface, (bState == JNI_TRUE)?1:0);

  jni_unlock(jEnv);
}

// -----[ getWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getWeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getWeight
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jlong jlResult;

  jni_lock(jEnv);

  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, 0);

  jlResult= (jlong) net_iface_get_metric(pIface, 0);

  return_jni_unlock(jEnv, jlResult);
}

// -----[ setWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setWeight
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setWeight
  (JNIEnv * jEnv, jobject joIface, jlong jlWeight)
{
  net_iface_t * pIface;
  int iResult;

  jni_lock(jEnv);

  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock2(jEnv);

  iResult= net_iface_set_metric(pIface, 0, (uint32_t) jlWeight, 0);
  if (iResult != ESUCCESS)
    throw_CBGPException(jEnv, "could not set weight (%s)",
			network_strerror(iResult));

  jni_unlock(jEnv);
}

// -----[ getHead ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getHead
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getHead
  (JNIEnv * jEnv, jobject joLink)
{
  net_iface_t * pLink;
  jobject joNode;

  jni_lock(jEnv);

  pLink= (net_iface_t *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, NULL);

  joNode= cbgp_jni_new_net_Node(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				pLink->src_node);

  return_jni_unlock(jEnv, joNode);
}

// -----[ getTail ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getTail
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Element;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getTail
  (JNIEnv * jEnv, jobject joLink)
{
  net_iface_t * pLink;
  jobject joElement= NULL;

  jni_lock(jEnv);

  pLink= (net_iface_t *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, NULL);

  switch (pLink->type) {
  case NET_IFACE_RTR:
    joElement= cbgp_jni_new_net_Node(jEnv,
				     NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				     pLink->dest.iface->src_node);
    break;

  case NET_IFACE_PTP:
    joElement= cbgp_jni_new_net_Node(jEnv,
				     NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				     pLink->dest.iface->src_node);
    break;

  case NET_IFACE_PTMP:
    joElement= cbgp_jni_new_net_Subnet(jEnv,
				       NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				       pLink->dest.subnet);
    break;

  default:
    abort();
  }

  return_jni_unlock(jEnv, joElement);
}

