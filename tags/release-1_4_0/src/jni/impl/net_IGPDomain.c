// ==================================================================
// @(#)net_IGPDomain.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/04/2006
// @lastdate 29/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/net_IGPDomain.h>
#include <jni/impl/net_Node.h>

#define CLASS_IGPDomain "be/ac/ucl/ingi/cbgp/net/IGPDomain"
#define CONSTR_IGPDomain "(Lbe/ac/ucl/ingi/cbgp/CBGP;II)V"

// -----[ cbgp_jni_new_net_IGPDomain ]-------------------------------
/**
 * This function creates a new instance of the net.IGPDomain object
 * from an IGP domain.
 */
jobject cbgp_jni_new_net_IGPDomain(JNIEnv * jEnv, jobject joCBGP,
				   SIGPDomain * pDomain)
{
  jobject joDomain;

  /* Create new IGPDomain object */
  if ((joDomain= cbgp_jni_new(jEnv, CLASS_IGPDomain,
			      CONSTR_IGPDomain,
			      joCBGP,
			      pDomain->uNumber,
			      pDomain->tType)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joDomain, pDomain);

  return joDomain;
}

// -----[ addNode ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_IGPDomain
 * Method:    addNode
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_IGPDomain_addNode
  (JNIEnv * jEnv, jobject joDomain, jstring jsAddr)
{
  SIGPDomain * pDomain;
  SNetNode * pNode; 
  net_addr_t tNetAddr;
  jobject joNode;

  jni_lock(jEnv);

  /* Get the domain */
  pDomain= (SIGPDomain *) jni_proxy_lookup(jEnv, joDomain);
  if (pDomain == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_address(jEnv, jsAddr, &tNetAddr) != 0)
    return_jni_unlock(jEnv, NULL);
  if ((pNode= node_create(tNetAddr)) == NULL) {
    cbgp_jni_throw_CBGPException(jEnv, "node could not be created");
    return_jni_unlock(jEnv, NULL);
  }

  if (network_add_node(pNode) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "node already exists");
    return_jni_unlock(jEnv, NULL);
  }

  igp_domain_add_router(pDomain, pNode);

  joNode= cbgp_jni_new_net_Node(jEnv, jni_proxy_get_CBGP(jEnv, joDomain),
				pNode); 

  return_jni_unlock(jEnv, joNode);
}

// -----[ _netDomainGetNodes ]---------------------------------------
static int _netDomainGetNodes(uint32_t uKey, uint8_t uKeyLen,
			      void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  SNetNode * pNode= (SNetNode * ) pItem;
  jobject joNode;

  if ((joNode= cbgp_jni_new_net_Node(pCtx->jEnv,
				     pCtx->joCBGP,
				     pNode)) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joNode);
}

// -----[ getNodes ]-------------------------------------------------
/**
 * Class:  be_ac_ucl_ingi_cbgp_net_IGPDomain
 * Method: getNodes
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_IGPDomain_getNodes
(JNIEnv * jEnv, jobject joDomain)
{
  jobject joVector;
  SJNIContext sCtx;
  SIGPDomain * pDomain= NULL;

  jni_lock(jEnv);

  pDomain= (SIGPDomain *) jni_proxy_lookup(jEnv, joDomain);
  if (pDomain == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= jni_proxy_get_CBGP(jEnv, joDomain);

  if (igp_domain_routers_for_each(pDomain, _netDomainGetNodes, &sCtx))
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ compute ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_IGPDomain
 * Method:    compute
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_IGPDomain_compute
  (JNIEnv * jEnv, jobject joDomain)
{
  SIGPDomain * pDomain= NULL;

  jni_lock(jEnv);

  pDomain= (SIGPDomain *) jni_proxy_lookup(jEnv, joDomain);
  if (pDomain == NULL)
    return_jni_unlock2(jEnv);

  if (igp_domain_compute(pDomain) != 0) {
    cbgp_jni_throw_CBGPException(jEnv, "could not compute IGP paths");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}
