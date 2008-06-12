// ==================================================================
// @(#)net_Link.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 18/11/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/net_Subnet.h>

#include <net/link.h>

#define CLASS_Link "be/ac/ucl/ingi/cbgp/net/Link"
#define CONSTR_Link "(Lbe/ac/ucl/ingi/cbgp/CBGP;"\
                    "Lbe/ac/ucl/ingi/cbgp/IPAddress;J)V"

// -----[ cbgp_jni_new_net_Link ]------------------------------------
/**
 * This function creates a new instance of the Link object from a CBGP
 * link.
 */
jobject cbgp_jni_new_net_Link(JNIEnv * jEnv, jobject joCBGP,
			      SNetLink * pLink)
{
  jobject joLink;

  /* Java proxy object already existing ? */
  joLink= jni_proxy_get(jEnv, pLink);
  if (joLink != NULL)
    return joLink;

  /* Convert link attributes to Java objects */
  jobject obj_IPAddress=
    cbgp_jni_new_IPAddress(jEnv, net_link_get_address(pLink));

  /* Check that the conversion was successful */
  if (obj_IPAddress == NULL)
    return NULL;

  /* Create new Link object */
  if ((joLink= cbgp_jni_new(jEnv, CLASS_Link, CONSTR_Link,
			    joCBGP,
			    obj_IPAddress,
			    (jlong) pLink->tDelay)) == NULL)
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
  (JNIEnv * jEnv, jobject joLink)
{
  SNetLink * pLink;
  jlong jlCapacity= 0;

  jni_lock(jEnv);

  /* Get link */
  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, -1);

  jlCapacity= net_link_get_capacity(pLink);

  return_jni_unlock(jEnv, jlCapacity);
}

// -----[ setCapacity ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setCapacity
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setCapacity
  (JNIEnv * jEnv, jobject joLink, jlong jlCapacity)
{
  SNetLink * pLink;

  jni_lock(jEnv);

  /* Get link */
  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock2(jEnv);

  net_link_set_capacity(pLink, jlCapacity);

  jni_unlock(jEnv);
}

// -----[ getLoad ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getLoad
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getLoad
  (JNIEnv * jEnv, jobject joLink)
{
  SNetLink * pLink;
  jlong jlLoad= 0;

  jni_lock(jEnv);

  /* Get link */
  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, -1);

  jlLoad= net_link_get_load(pLink);

  return_jni_unlock(jEnv, jlLoad);
}

// -----[ getState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getState
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getState
  (JNIEnv * jEnv, jobject joLink)
{
  SNetLink * pLink;
  jboolean jbState;

  jni_lock(jEnv);
  
  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jbState= net_link_get_state(pLink, NET_LINK_FLAG_UP)?JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jbState);
}

// -----[ setState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setState
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setState
  (JNIEnv * jEnv, jobject joLink, jboolean bState)
{
  SNetLink * pLink;

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock2(jEnv);
    
  net_link_set_state(pLink, NET_LINK_FLAG_UP, (bState == JNI_TRUE)?1:0);

  jni_unlock(jEnv);
}

// -----[ getWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    getWeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_getWeight
  (JNIEnv * jEnv, jobject joLink)
{
  SNetLink * pLink;
  jlong jlResult;

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, 0);

  jlResult= (jlong) net_link_get_weight(pLink, 0);

  return_jni_unlock(jEnv, jlResult);
}

// -----[ setWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Link
 * Method:    setWeight
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Link_setWeight
  (JNIEnv * jEnv, jobject joLink, jlong jlWeight)
{
  SNetLink * pLink;

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock2(jEnv);

  net_link_set_weight(pLink, 0, (uint32_t) jlWeight);

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
  SNetLink * pLink;
  jobject joNode;

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, NULL);

  joNode= cbgp_jni_new_net_Node(jEnv,
				jni_proxy_get_CBGP(jEnv, joLink),
				pLink->pSrcNode);

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
  SNetLink * pLink;
  jobject joElement= NULL;

  jni_lock(jEnv);

  pLink= (SNetLink *) jni_proxy_lookup(jEnv, joLink);
  if (pLink == NULL)
    return_jni_unlock(jEnv, NULL);

  switch (pLink->uType) {
  case NET_LINK_TYPE_ROUTER:
    joElement= cbgp_jni_new_net_Node(jEnv,
				     jni_proxy_get_CBGP(jEnv, joLink),
				     pLink->tDest.pNode);
    break;
  case NET_LINK_TYPE_TRANSIT:
  case NET_LINK_TYPE_STUB:
    joElement= cbgp_jni_new_net_Subnet(jEnv,
				       jni_proxy_get_CBGP(jEnv, joLink),
				       pLink->tDest.pSubnet);
    break;
  }

  return_jni_unlock(jEnv, joElement);
}
