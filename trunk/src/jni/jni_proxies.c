// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 31/03/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni.h>

#include <libgds/hash.h>
#include <libgds/memory.h>

#define JNI_PROXY_HASH_SIZE 1000

static SHash * pHashCode2Object= NULL;

typedef struct {
  jint jiHashCode;
  void * pObject;
} SHashCodeObject;

// -----[ _jni_proxy_compare ]---------------------------------------
static int _jni_proxy_compare(void * pElt1, void * pElt2, uint32_t uEltSize)
{
  SHashCodeObject * pHashObj1= (SHashCodeObject *) pElt1;
  SHashCodeObject * pHashObj2= (SHashCodeObject *) pElt2;
  if (pHashObj1->jiHashCode > pHashObj2->jiHashCode)
    return 1;
  else if (pHashObj2->jiHashCode < pHashObj1->jiHashCode)
    return -1;
  else
    return 0;
}

// -----[ _jni_proxy_destroy ]---------------------------------------
static void _jni_proxy_destroy(void * pElt)
{
  FREE(pElt);
}

// -----[ _jni_proxy_compute ]---------------------------------------
static uint32_t _jni_proxy_compute(void * pElt, uint32_t uHashSize)
{
  SHashCodeObject * pHashObj= (SHashCodeObject *) pElt;
  return pHashObj->jiHashCode % uHashSize;
}

// -----[ jni_proxy_add ]--------------------------------------------
/**
 * Add a mapping between a JNI proxy and a native object.
 */
void jni_proxy_add(jint jiHashCode, void * pObject)
{
  SHashCodeObject * pHashObj=
    (SHashCodeObject *) MALLOC(sizeof(SHashCodeObject));
  pHashObj->jiHashCode= jiHashCode;
  pHashObj->pObject= pObject;
  //fprintf(stderr, "JNI::proxy_add(%d, %p)\n", jiHashCode, pObject);
  if (pHashCode2Object == NULL) {
    pHashCode2Object= hash_init(JNI_PROXY_HASH_SIZE,
				0,
				_jni_proxy_compare,
				_jni_proxy_destroy,
				_jni_proxy_compute);
  }
  hash_add(pHashCode2Object, pHashObj);
}

// -----[ jni_proxy_remove ]-----------------------------------------
/**
 *
 */
void jni_proxy_remove(jint jiHashCode)
{
  //fprintf(stderr, "JNI::proxy_remove(%d)\n", jiHashCode);
}

// -----[ jni_proxy_lookup ]-----------------------------------------
/**
 *
 */
void * jni_proxy_lookup(jint jiHashCode)
{
  SHashCodeObject sObject;
  SHashCodeObject * pObject;
  sObject.jiHashCode= jiHashCode;
  //fprintf(stderr, "JNI::proxy_lookup(%d)\n", jiHashCode);
  pObject= hash_search(pHashCode2Object, &sObject);
  if (pObject != NULL)
    return pObject->pObject;
  return NULL;
}