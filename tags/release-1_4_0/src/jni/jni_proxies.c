// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 29/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <jni.h>
#include <string.h>

#include <libgds/hash.h>
#include <libgds/memory.h>

#include <jni/jni_base.h>
#include <jni/jni_util.h>

//#define DEBUG_LOCK
//#define DEBUG_PROXIES
//#define ERROR_PROXIES

#define JNI_PROXY_HASH_SIZE 1000

#define CLASS_ProxyObject "be/ac/ucl/ingi/cbgp/ProxyObject"
#define METHOD_ProxyObject_getCBGP "()Lbe/ac/ucl/ingi/cbgp/CBGP;"

static SHash * pHashCode2Object= NULL;

typedef struct {
  jint jiHashCode;
  void * pObject;
#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
  char * pcClassName;
#endif /* ERROR_PROXIES || DEBUG_PROXIES */
} SHashCodeObject;

#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
static char * get_class_name(JNIEnv * jEnv, jobject joObject)
{
  jclass jcClass;
  jstring jsName;
  const char * str;
  char * pcResult;

  assert((jcClass= (*jEnv)->GetObjectClass(jEnv, joObject)) != NULL);
  
  assert((jsName= (jstring) cbgp_jni_call_Object(jEnv, jcClass, "getName",
    "()Ljava/lang/String;")) != NULL);

  str= (*jEnv)->GetStringUTFChars(jEnv, jsName, NULL);
  if (str == NULL)
    return NULL; /* OutOfMemoryError already thrown */

  pcResult= strdup(str);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsName, str);

  return pcResult;
}
#endif /* ERROR_PROXIES || DEBUG_PROXIES */

// -----[ _jni_proxy_compare ]---------------------------------------
static int _jni_proxy_compare(void * pElt1, void * pElt2, uint32_t uEltSize)
{
  SHashCodeObject * pHashObj1= (SHashCodeObject *) pElt1;
  SHashCodeObject * pHashObj2= (SHashCodeObject *) pElt2;
  if (pHashObj1->jiHashCode > pHashObj2->jiHashCode)
    return 1;
  else if (pHashObj1->jiHashCode < pHashObj2->jiHashCode)
    return -1;
  else {

#ifdef DEBUG_PROXIES
    fprintf(stderr, "debug: \033[33;1mwarning, same hash code (%d==%d)\033[0m\n", (int) pHashObj1->jiHashCode, (int) pHashObj2->jiHashCode);
#endif /* DEBUG_PROXIES */

    return 0;
  }
}

// -----[ _jni_proxy_destroy ]---------------------------------------
static void _jni_proxy_destroy(void * pElt)
{
#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
  SHashCodeObject * pHashObj= (SHashCodeObject *) pElt;
  fprintf(stderr, "debug: remove \"%s\" (key:%d)\n",
	  pHashObj->pcClassName,
	  (int) pHashObj->jiHashCode);
  free(pHashObj->pcClassName);
#endif /* ERROR_PROXIES || DEBUG_PROXIES */

  FREE(pElt);
}

// -----[ _jni_proxy_compute ]---------------------------------------
static uint32_t _jni_proxy_compute(const void * pElt, const uint32_t uHashSize)
{
  SHashCodeObject * pHashObj= (SHashCodeObject *) pElt;
  if (pHashObj->jiHashCode < 0) {
    fprintf(stderr, "warning: hashcode < 0 (%d)\n",
	    (int) pHashObj->jiHashCode);
  }
  return pHashObj->jiHashCode % uHashSize;
}

// -----[ jni_proxy_add ]--------------------------------------------
/**
 * Add a mapping between a JNI proxy and a native object.
 */
void jni_proxy_add(JNIEnv * jEnv, jobject joObject, void * pObject)
{
  SHashCodeObject * pHashObj;
  jclass jcClass;
  jfieldID jfFieldID;
  
  // Set the object's hash code
  if ((jcClass= (*jEnv)->GetObjectClass(jEnv, joObject)) == NULL) {
    fprintf(stderr, "Fatal error: couldn't get object's class.\n");
    abort();
  }
  if ((jfFieldID= (*jEnv)->GetFieldID(jEnv, jcClass, "hash_code", "I")) == NULL) {
    fprintf(stderr, "Fatal error: couldn't get class's method ID.\n");
    abort();
  }

  // NOTE: This is a very tricky thing that I shouldn't do: cast a
  // pointer to an int (won't work under 64-bits platform). I'm
  // just going on a proof of concept...
  (*jEnv)->SetIntField(jEnv, joObject, jfFieldID, (int) pObject);

  pHashObj=
    (SHashCodeObject *) MALLOC(sizeof(SHashCodeObject));
  pHashObj->jiHashCode= jni_Object_hashCode(jEnv, joObject);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy add [j-obj:%p, c-obj:%p, key:\033[1m%d\033[0m]\n", joObject,
	  pObject, (int) pHashObj->jiHashCode);
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  pHashObj->pObject= pObject;
#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
  pHashObj->pcClassName= get_class_name(jEnv, joObject);
  fprintf(stderr, "debug: add \"%s\" (key:%d)\n",
	  pHashObj->pcClassName,
	  (int) pHashObj->jiHashCode);
  fflush(stderr);
#endif /* ERROR_PROXIES || DEBUG_PROXIES */
  if (pHashCode2Object == NULL) {
    pHashCode2Object= hash_init(JNI_PROXY_HASH_SIZE,
				0,
				_jni_proxy_compare,
				_jni_proxy_destroy,
				_jni_proxy_compute);
  }
  hash_add(pHashCode2Object, pHashObj);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy added\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */
}

// -----[ jni_proxy_remove ]-----------------------------------------
/**
 *
 */
void jni_proxy_remove(JNIEnv * jEnv, jobject joObject)
{
  SHashCodeObject sObject;

  sObject.jiHashCode= jni_Object_hashCode(jEnv, joObject);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "\033[1mdebug: proxy remove [j-obj:%p; key:\033[1m%d\033[0m]\033[0m\n", joObject,
	  (int) sObject.jiHashCode);
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  hash_del(pHashCode2Object, &sObject);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "\033[1mdebug: proxy removed\033[0m\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */
}

// -----[ jni_proxy_lookup ]-----------------------------------------
/**
 *
 */
void * jni_proxy_lookup(JNIEnv * jEnv, jobject joObject)
{
  SHashCodeObject sObject;
  SHashCodeObject * pObject;
#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
  char * pcSearchedClassName;
#endif /* ERROR_PROXIES || DEBUG_PROXIES */

  sObject.jiHashCode= jni_Object_hashCode(jEnv, joObject);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy lookup [j-obj:%p, key:\033[1m%d\033[0m]\n", joObject,
	  (int) sObject.jiHashCode);
#endif
#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
  pcSearchedClassName= get_class_name(jEnv, joObject);
#endif /* ERROR_PROXIES || DEBUG_PROXIES */
#ifdef DEBUG_PROXIES
  fprintf(stderr, "  --> class-name: \"%s\"\n", pcSearchedClassName);
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  pObject= hash_search(pHashCode2Object, &sObject);
  if (pObject == NULL) {

#ifdef DEBUG_PROXIES
    fprintf(stderr, "debug: proxy lookup failed\n");
    fflush(stderr);
#endif /* DEBUG_PROXIES */

    cbgp_jni_throw_CBGPException(jEnv, "JNI proxy lookup error");
    return NULL;
  }

#ifdef DEBUG_PROXIES
  fprintf(stderr, "  --> class-name: \"%s\"\n", pObject->pcClassName);
#endif /* DEBUG_PROXIES */
#if defined(ERROR_PROXIES) || defined(DEBUG_PROXIES)
  if (strcmp(pcSearchedClassName, pObject->pcClassName) != 0) {
    fprintf(stderr, "debug: \033[33;1merror, class-name mismatch\033[0m\n");
    fprintf(stderr, "       --> searched: \"\033[33;1m%s\033[0m\" (key:\033[33;1m%d\033[0m)\n", pcSearchedClassName, (int) sObject.jiHashCode);
    fprintf(stderr, "       --> found: \"\033[33;1m%s\033[0m\" (key:\033[33;1m%d\033[0m)\n", pObject->pcClassName, (int) pObject->jiHashCode);
  }
  free(pcSearchedClassName);
#endif /* ERROR_PROXIES || DEBUG_PROXIES */
#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy lookup ok [c-obj:%p, key:\033[1m%d\033[0m]\n", pObject->pObject,
	  (int) pObject->jiHashCode);
  if (pObject->jiHashCode != sObject.jiHashCode) {
    fprintf(stderr, "debug: \033[33;1mwarning, key mismatch\033[0m\n");
  }
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  return pObject->pObject;
}

// -----[ jni_proxy_get_CBGP ]---------------------------------------
/**
 *
 */
inline jobject jni_proxy_get_CBGP(JNIEnv * jEnv, jobject joObject)
{
  return cbgp_jni_call_Object(jEnv, joObject, "getCBGP",
			       METHOD_ProxyObject_getCBGP);
}

// -----[ _jni_unregister ]------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_ProxyObject__1jni_1unregister
  (JNIEnv * jEnv, jobject joObject)
{
  jni_proxy_remove(jEnv, joObject);
}


/////////////////////////////////////////////////////////////////////
//
// JNI Critical Section management functions
//
/////////////////////////////////////////////////////////////////////

static jobject joLockObj= NULL;

// -----[ jni_init_lock ]--------------------------------------------
void jni_init_lock(jobject joObj)
{
#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: init monitor [%p]\n", joObj);
  fflush(stderr);
#endif /* DEBUG_LOCK */

  joLockObj= joObj;
}

// -----[ jni_lock ]-------------------------------------------------
void jni_lock(JNIEnv * jEnv)
{
#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor enter [%p]\n", joLockObj);
  fflush(stderr);
#endif /* DEBUG_LOCK */

  if ((*jEnv)->MonitorEnter(jEnv, joLockObj) != JNI_OK) {
    fprintf(stderr, "ERROR: could not enter JNI monitor\n");
    abort();
  }

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor entered\n");
  fflush(stderr);
#endif /* DEBUG_LOCK */
}

// -----[ jni_unlock ]-----------------------------------------------
void jni_unlock(JNIEnv * jEnv)
{
#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor exit [%p]\n", joLockObj);
  fflush(stderr);
#endif /* DEBUG_LOCK */

  (*jEnv)->ExceptionClear(jEnv);
  if ((*jEnv)->MonitorExit(jEnv, joLockObj) != JNI_OK) {
    fprintf(stderr, "ERROR: could not exit JNI monitor\n");
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
      (*jEnv)->ExceptionDescribe(jEnv); 
      (*jEnv)->ExceptionClear(jEnv);
    }
    fprintf(stderr, "aborting.\n");
    abort();
  }

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor exited\n");
  fflush(stderr);
#endif /* DEBUG_LOCK */
}
