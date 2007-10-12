// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/03/2006
// @lastdate 11/10/2007
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
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>

//#define DEBUG_LOCK
//#define DEBUG_PROXIES

#define JNI_PROXY_HASH_SIZE 1000

#define CLASS_ProxyObject "be/ac/ucl/ingi/cbgp/ProxyObject"
#define METHOD_ProxyObject_getCBGP "()Lbe/ac/ucl/ingi/cbgp/CBGP;"

static SHash * pC2JHash= NULL;
static SHash * pJ2CHash= NULL;
static unsigned long ulProxySeqNum= 0;

// -----[ SHashCodeObject ]-----
/** Maintains the mapping between a CObj and a JObj. */
typedef struct {
  void * pObject;   // Identifier for CObj
  jobject joObject; // Identifier for JObj (weak global reference)
  unsigned long int ulObjectId;

#if defined(DEBUG_PROXIES)
  char * pcClassName;
#endif /* DEBUG_PROXIES */

} SHashCodeObject;
typedef SHashCodeObject SJNIProxy;

// -----[ jni_abort ]------------------------------------------------
void jni_abort(JNIEnv * jEnv)
{
  if ((*jEnv)->ExceptionOccurred(jEnv)) {
    (*jEnv)->ExceptionDescribe(jEnv); 
    (*jEnv)->ExceptionClear(jEnv);
    fflush(stderr);
  }
  fprintf(stderr, "aborting.\n");
  abort();
}

// -----[ get_class_name ]-------------------------------------------
//#if defined(DEBUG_PROXIES)
char * get_class_name(JNIEnv * jEnv, jobject joObject)
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
//#endif /* DEBUG_PROXIES */

// -----[ _jni_proxy_Object_get_id ]---------------------------------
/**
 *
 */
static jlong _jni_proxy_Object_get_id(JNIEnv * jEnv, jobject joObject)
{
  jclass jcClass;
  jfieldID jfFieldID;

  jcClass= (*jEnv)->GetObjectClass(jEnv, joObject);
  if (jcClass == NULL) {
    fprintf(stderr, "Fatal error: couldn't get object's class.\n");
    jni_abort(jEnv);
  }

  jfFieldID= (*jEnv)->GetFieldID(jEnv, jcClass, "objectId", "J");
  if (jfFieldID == NULL) {
    fprintf(stderr, "Fatal error: couldn't get class's field ID.\n");
    jni_abort(jEnv);
  }

  return (*jEnv)->GetLongField(jEnv, joObject, jfFieldID);
}

// -----[ _jni_proxy_Object_set_id ]---------------------------------
/**
 *
 */
static void _jni_proxy_Object_set_id(JNIEnv * jEnv, jobject joObject,
				     jlong jlId)
{
  jclass jcClass;
  jfieldID jfFieldID;

  jcClass= (*jEnv)->GetObjectClass(jEnv, joObject);
  if (jcClass == NULL) {
    fprintf(stderr, "Fatal error: couldn't get object's class.\n");
    jni_abort(jEnv);
  }

  jfFieldID= (*jEnv)->GetFieldID(jEnv, jcClass, "objectId", "J");
  if (jfFieldID == NULL) {
    fprintf(stderr, "Fatal error: couldn't get class's field ID.\n");
    jni_abort(jEnv);
  }

  (*jEnv)->SetLongField(jEnv, joObject, jfFieldID, jlId);
}

// -----[ _jni_proxy_create ]----------------------------------------
static SJNIProxy * _jni_proxy_create(JNIEnv * jEnv,
				     void * pObject,
				     jobject joObject)
{
  SJNIProxy * pProxy= MALLOC(sizeof(SJNIProxy));
  pProxy->pObject= pObject;
  pProxy->joObject= (*jEnv)->NewWeakGlobalRef(jEnv, joObject);;
  pProxy->ulObjectId= ++ulProxySeqNum;

  // NOTE: We can't use Java's hashcode since it is allowed to return
  // an equal value for two different objects (sadly unicity isn't
  // guaranteed for Java hashcodes). Therefore, to provide the
  // mapping from the JObj to the CObj, we store a sequence number
  // which is then used as a key to retrieve the CObj's pointer.
  _jni_proxy_Object_set_id(jEnv, joObject, pProxy->ulObjectId);

#if defined(DEBUG_PROXIES)
  pProxy->pcClassName= get_class_name(jEnv, joObject);
#endif /* DEBUG_PROXIES */

  return pProxy;
}

// -----[ _jni_proxy_dump ]------------------------------------------
static void _jni_proxy_dump(SJNIProxy * pProxy)
{
  fprintf(stderr, "[c-obj:%p/j-obj:%p/id:%lu",
	  pProxy->pObject, pProxy->joObject, pProxy->ulObjectId);

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "/class:%s", pProxy->pcClassName);
#endif /* DEBUG_PROXIES */

  fprintf(stderr, "]");
}

// -----[ _jni_proxy_c2j_cmp ]---------------------------------------
static int _jni_proxy_c2j_cmp(void * pElt1, void * pElt2,
			      uint32_t uEltSize)
{
  SHashCodeObject * pHashObj1= (SHashCodeObject *) pElt1;
  SHashCodeObject * pHashObj2= (SHashCodeObject *) pElt2;

  if (pHashObj1->pObject > pHashObj2->pObject)
    return 1;
  else if (pHashObj1->pObject < pHashObj2->pObject)
    return -1;
  else
    return 0;
}

// -----[ _jni_proxy_j2c_cmp ]---------------------------------------
static int _jni_proxy_j2c_cmp(void * pElt1, void * pElt2,
			      uint32_t uEltSize)
{
  SHashCodeObject * pHashObj1= (SHashCodeObject *) pElt1;
  SHashCodeObject * pHashObj2= (SHashCodeObject *) pElt2;

  if (pHashObj1->ulObjectId > pHashObj2->ulObjectId)
    return 1;
  else if (pHashObj1->ulObjectId < pHashObj2->ulObjectId)
    return -1;
  else
    return 0;
}

// -----[ _jni_proxy_c2j_destroy ]-----------------------------------
static void _jni_proxy_c2j_destroy(void * pElt)
{
  SJNIProxy * pProxy= (SJNIProxy *) pElt;

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "debug: invalidate ");
  _jni_proxy_dump(pProxy);
  fprintf(stderr, "\n");
#endif /* DEBUG_PROXIES */

  pProxy->pObject= NULL;
}

// -----[ _jni_proxy_j2c_destroy ]-----------------------------------
static void _jni_proxy_j2c_destroy(void * pElt)
{
  SJNIProxy * pProxy= (SJNIProxy *) pElt;

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "debug: destroy ");
  _jni_proxy_dump(pProxy);
  fprintf(stderr, "\n");
#endif /* DEBUG_PROXIES */

  //(*jEnv)->DeleteWeakGlobalRef(jEnv, pProxy->joObject);
  FREE(pProxy);
}

// -----[ _jni_proxy_c2j_compute ]-----------------------------------
static uint32_t _jni_proxy_c2j_compute(const void * pElt,
				       const uint32_t uHashSize)
{
  SHashCodeObject * pHashObj= (SHashCodeObject *) pElt;

  return ((unsigned long int) pHashObj->pObject) % uHashSize;
}

// -----[ _jni_proxy_j2c_compute ]-----------------------------------
static uint32_t _jni_proxy_j2c_compute(const void * pElt,
				       const uint32_t uHashSize)
{
  SHashCodeObject * pHashObj= (SHashCodeObject *) pElt;

  return pHashObj->ulObjectId % uHashSize;
}

// -----[ jni_proxy_add ]--------------------------------------------
/**
 * Add a mapping between a JNI proxy and a native object.
 */
void jni_proxy_add(JNIEnv * jEnv, jobject joObject, void * pObject)
{
  SJNIProxy * pProxy= _jni_proxy_create(jEnv, pObject, joObject);

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "debug: proxy add ");
  _jni_proxy_dump(pProxy);
  fprintf(stderr, "\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  if (hash_add(pC2JHash, pProxy) != pProxy) {
    fprintf(stderr, "*** WARNING: same object already referenced (C2J) ! ***\n");
    _jni_proxy_dump(pProxy);
    fprintf(stderr, "\n");
    abort();
  }
  if (hash_add(pJ2CHash, pProxy) != pProxy) {
    fprintf(stderr, "*** WARNING: same object already referenced (J2C) ! ***\n");
    _jni_proxy_dump(pProxy);
    fprintf(stderr, "\n");
    abort();
  }

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
  SJNIProxy sProxy, * pProxy;
  //char * pcSearchedClassName;

  sProxy.ulObjectId= _jni_proxy_Object_get_id(jEnv, joObject);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy remove [j-obj:%p/key:%lu]\n",
	  joObject, sProxy.ulObjectId);
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  /*
  pcSearchedClassName= get_class_name(jEnv, joObject);
  fprintf(stderr, "  --> class-name: \"%s\"\n", pcSearchedClassName);
  fflush(stderr);
  free(pcSearchedClassName);
  */

  pProxy= hash_search(pJ2CHash, &sProxy);
  if (pProxy == NULL)
    jni_abort(jEnv);

  /*
    if (!(*jEnv)->IsSameObject(jEnv, joObject, pProxy->joObject)) {
    fprintf(stderr, "NOK (not the same object) ***\n");
    jni_abort(jEnv);
    }
  */

  if (pC2JHash != NULL)
    hash_del(pC2JHash, pProxy);
  hash_del(pJ2CHash, pProxy);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy removed\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */
}

// -----[ jni_proxy_lookup ]-----------------------------------------
/**
 *
 */
void * jni_proxy_lookup(JNIEnv * jEnv, jobject joObject)
{
#if defined(DEBUG_PROXIES)
  char * pcSearchedClassName;
#endif /* DEBUG_PROXIES */
  unsigned long int ulObjectId= _jni_proxy_Object_get_id(jEnv, joObject);
  SHashCodeObject sHashObject;
  SHashCodeObject * pHashObject;

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy lookup [j-obj:%p/key:%lu]\n",
	  joObject, ulObjectId);
#endif

#if defined(DEBUG_PROXIES)
  pcSearchedClassName= get_class_name(jEnv, joObject);
  fprintf(stderr, "  --> class-name: \"%s\"\n", pcSearchedClassName);
  fflush(stderr);
  free(pcSearchedClassName);
#endif /* DEBUG_PROXIES */

  if (pJ2CHash == NULL) {
    fprintf(stderr, "Error: proxy lookup while C-BGP not initialized.\n");
    abort();
  }

  sHashObject.ulObjectId= ulObjectId;
  pHashObject= hash_search(pJ2CHash, &sHashObject);
  if (pHashObject == NULL) {
    fprintf(stderr, "Error: during lookup, reported ID has no mapping\n");
    abort();
  }

  return pHashObject->pObject;

}

// -----[ jni_proxy_get ]--------------------------------------------
/**
 *
 */
jobject jni_proxy_get(JNIEnv * jEnv, void * pObject)
{
  SJNIProxy sProxy;
  SJNIProxy * pProxy;

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy get [c-obj:%p]\n", pObject);
#endif

  sProxy.pObject= pObject;
  pProxy= hash_search(pC2JHash, &sProxy);
  if (pProxy == NULL) {
#ifdef DEBUG_PROXIES
    fprintf(stderr, "debug: proxy get failed\n");
    fflush(stderr);
#endif /* DEBUG_PROXIES */
    return NULL;
  }

#ifdef DEBUG_PROXIES
  fprintf(stderr, "  --> proxy: ");
  _jni_proxy_dump(pProxy);
  fprintf(stderr, "\n");
#endif /* DEBUG_PROXIES */

  /*#if defined(DEBUG_PROXIES)
  if (strcmp(pcSearchedClassName, pHashObject->pcClassName) != 0) {
    fprintf(stderr, "debug: \033[33;1merror, class-name mismatch\033[0m\n");
    fprintf(stderr, "       --> searched: \"\033[33;1m%s\033[0m\" (key:\033[33;1m%p\033[0m)\n", pcSearchedClassName, sHashObject.pObject);
    fprintf(stderr, "       --> found: \"\033[33;1m%s\033[0m\" (key:\033[33;1m%p\033[0m)\n", pObject->pcClassName, pHashObject->pObject);
  }
  #endif*/ /* DEBUG_PROXIES */

  /** TODO: need to check if weak reference has not been
   * garbage-collected ! This can be done by relying on the
   * "IsSameObject" method...
   *
   * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   */
  if ((*jEnv)->IsSameObject(jEnv, pProxy->joObject, NULL)) {
    fprintf(stderr, "Error: object was garbage-collected.\n");
    abort();
  }

  return pProxy->joObject;
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
 * This is called when a Java proxy object is garbage-collected.
 *
 * This must only happen when the pJ2CHash is allocated and the
 * jni_lock_init() has been called. The CBGP.init() function takes
 * care of this.
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_ProxyObject__1jni_1unregister
  (JNIEnv * jEnv, jobject joObject)
{
  jni_lock(jEnv);
  jni_proxy_remove(jEnv, joObject);
  jni_unlock(jEnv);
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION / FINALIZATION OF THE C->JAVA AND JAVA->C MAPPINGS
//
/////////////////////////////////////////////////////////////////////

// -----[ _jni_proxies_init ]----------------------------------------
/**
 * This function initializes the C->Java and Java->C mappings. It can
 * safely be used upon a CBGP JNI session creating or upon the JNI
 * library loading. The CBGP.init() JNI function currently takes care
 * of this call.
 */
void _jni_proxies_init()
{
  /** Note: this hash should be persistent accross multiple C-BGP
   * classes initialization/finalization since proxy objects can be
   * garbage-collected by the Java VM after the C-BGP object has been
   * removed.
   */
  if (pC2JHash == NULL) {
    pC2JHash= hash_init(JNI_PROXY_HASH_SIZE,
			0,
			_jni_proxy_c2j_cmp,
			_jni_proxy_c2j_destroy,
			_jni_proxy_c2j_compute);
  }
  if (pJ2CHash == NULL) {
    pJ2CHash= hash_init(JNI_PROXY_HASH_SIZE,
			0,
			_jni_proxy_j2c_cmp,
			_jni_proxy_j2c_destroy,
			_jni_proxy_j2c_compute);
  }
}

// -----[ _jni_proxies_invalidate ]----------------------------------
/**
 * This function invalidates all the C->Java mappings. This function
 * must be called at the end of a C-BGP session. The CBGP.destroy()
 * JNI function takes care of this call.
 */
void _jni_proxies_invalidate()
{
  hash_destroy(&pC2JHash);
}

// -----[ _jni_proxies_destroy ]-------------------------------------
/**
 * This function frees all the C->Java and Java->C mappings. This
 * function must only be called when the JNI library is unloaded. The
 * JNI_OnUnload function takes care of this call.
 */
void _jni_proxies_destroy()
{
  hash_destroy(&pC2JHash);
  hash_destroy(&pJ2CHash);
}


/////////////////////////////////////////////////////////////////////
//
// JNI CRITICAL SECTION MANAGEMENT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

static jobject joLockObj= NULL;

// -----[ _jni_lock_init ]-------------------------------------------
/**
 * This function initializes the C-BGP JNI locking system. It is
 * responsible for creating a globally reference object which will be
 * used for locking by all the JNI functions. This function can safely
 * be used upon a C-BGP JNI session creation of upon a JNI library
 * loading. The JNI_OnLoad function currently takes care of this call.
 */
void _jni_lock_init(JNIEnv * jEnv)
{
  jclass jcClass;

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: init monitor\n");
  fflush(stderr);
#endif /* DEBUG_LOCK */

  if (joLockObj == NULL) {
    jcClass= (*jEnv)->FindClass(jEnv, "java/lang/Object");
    if (jcClass == NULL)
      jni_abort(jEnv);
    joLockObj= (*jEnv)->AllocObject(jEnv, jcClass);
    if (joLockObj == NULL)
      jni_abort(jEnv);
    joLockObj= (*jEnv)->NewGlobalRef(jEnv, joLockObj);
    if (joLockObj == NULL)
      jni_abort(jEnv);

#ifdef DEBUG_LOCK
    fprintf(stderr, "debug: global lock created [%p]\n", joLockObj);
    fflush(stderr);
#endif /* DEBUG_LOCK */

  }
}

// -----[ _jni_lock_destroy ]----------------------------------------
/**
 * This function terminates the C-BGP JNI locking system. It removes
 * the global reference towards the dummy monitored object. The
 * JNI_OnUnload function takes care of this call.
 */
void _jni_lock_destroy(JNIEnv * jEnv)
{
  (*jEnv)->DeleteGlobalRef(jEnv, joLockObj);
  joLockObj= NULL;
}

// -----[ jni_lock ]-------------------------------------------------
void jni_lock(JNIEnv * jEnv)
{
#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor enter [%p]\n", joLockObj);
  fflush(stderr);
#endif /* DEBUG_LOCK */

  if (joLockObj == NULL)
    jni_abort(jEnv);

  if ((*jEnv)->MonitorEnter(jEnv, joLockObj) != JNI_OK) {
    fprintf(stderr, "ERROR: could not enter JNI monitor\n");
    jni_abort(jEnv);
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

  if (joLockObj == NULL)
    jni_abort(jEnv);

  if ((*jEnv)->MonitorExit(jEnv, joLockObj) != JNI_OK) {
    fprintf(stderr, "ERROR: could not exit JNI monitor\n");
    jni_abort(jEnv);
  }

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor exited\n");
  fflush(stderr);
#endif /* DEBUG_LOCK */
}
