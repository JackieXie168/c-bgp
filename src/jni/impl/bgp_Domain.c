// ==================================================================
// @(#)bgp_Domain.c
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
#include <jni/impl/bgp_Domain.h>
#include <jni/impl/bgp_Router.h>

#include <bgp/as.h>
#include <bgp/domain.h>

#define CLASS_BGPDomain "be/ac/ucl/ingi/cbgp/bgp/Domain"
#define CONSTR_BGPDomain "(Lbe/ac/ucl/ingi/cbgp/CBGP;I)V"

// -----[ cbgp_jni_new_bgp_Domain ]-----------------------------------
/**
 * This function creates a new instance of the bgp.Domain object from a
 * BGP domain.
 */
jobject cbgp_jni_new_bgp_Domain(JNIEnv * jEnv, jobject joCBGP,
				SBGPDomain * pDomain)
{
  jobject joDomain;

  /* Create new BGPDomain object */
  if ((joDomain= cbgp_jni_new(jEnv, CLASS_BGPDomain, CONSTR_BGPDomain,
			      joCBGP,
			      (jint) pDomain->uNumber)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joDomain, pDomain);

  return joDomain;
}

// -----[ _proxy_finalize ]------------------------------------------
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Domain__1proxy_1finalize
(JNIEnv * jEnv, jobject joObject)
{
  //jint jiHashCode= jni_Object_hashCode(jEnv, joObject);
  //fprintf(stderr, "JNI::net_Link__proxy_finalize [key=%d]\n", jiHashCode);
}

// -----[ _bgpDomainGetRouters ]-------------------------------------
static int _bgpDomainGetRouters(uint32_t uKey, uint8_t uKeyLen,
				void * pItem, void * pContext)
{
  SJNIContext * pCtx= (SJNIContext *) pContext;
  SBGPRouter * pRouter= (SBGPRouter * ) pItem;
  jobject joRouter;

  if ((joRouter= cbgp_jni_new_bgp_Router(pCtx->jEnv,
					 pCtx->joCBGP,
					 pRouter)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRouter);
}

// -----[ getRouters ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Domain
 * Method:    getRouters
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Domain_getRouters
  (JNIEnv * jEnv, jobject joDomain)
{
  jobject joVector= NULL;
  SJNIContext sCtx;
  SBGPDomain * pDomain;

  jni_lock(jEnv);

  pDomain= (SBGPDomain *) jni_proxy_lookup(jEnv, joDomain);
  if (pDomain == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= jni_proxy_get_CBGP(jEnv, joDomain);

  if (bgp_domain_routers_for_each(pDomain, _bgpDomainGetRouters, &sCtx))
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ rescan ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Domain
 * Method:    rescan
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Domain_rescan
  (JNIEnv * jEnv, jobject joDomain)
{
  SBGPDomain * pDomain;

  jni_lock(jEnv);

  pDomain= (SBGPDomain *) jni_proxy_lookup(jEnv, joDomain);
  if (pDomain == NULL)
    return_jni_unlock2(jEnv);

  bgp_domain_rescan(pDomain);

  jni_unlock(jEnv);
}
