// ==================================================================
// @(#)jni_base.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/03/2006
// @lastdate 27/03/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni/jni_base.h>

/////////////////////////////////////////////////////////////////////
// Helper functions for Java objects creation and method calls
/////////////////////////////////////////////////////////////////////

// -----[ cbgp_jni_new ]---------------------------------------------
/**
 * Creates a new Java Object.
 *
 * THROWS
 * - ClassFormatError: [FindClass]
 *     if the class data does not specify a valid class.
 * - ClassCircularityError: [FindClass]
 *     if a class or interface would be its own superclass or
 *     superinterface.
 * - NoClassDefFoundError: [FindClass]
 *     if no definition for a requested class or interface can be found.
 * - OutOfMemoryError: [FindClass, GetMethodID, NewObjectV]
 *     if the system runs out of memory.
 * - NoSuchMethodError: [GetMethodID]
 *     if the specified method cannot be found.
 * - ExceptionInInitializerError: [GetMethodID]
 *     if the class initializer fails due to an exception.
 * - InstantiationException: [NewObjectV]
 *     if the class is an interface or an abstract class.
 * - <*>: [NewObjectV]
 *     if any exception <*> is thrown by the constructor.
 *
 * Note: the function does not check if an exception is thrown. It
 * will rather rely on the return value of each function call
 * (FindClass, GetMethodID and NewObjectV). In case a value of NULL is
 * returned by any of these functions, 'cbgp_jni_new' will stop with a
 * return value of NULL.
 */
jobject cbgp_jni_new(JNIEnv * env,
		     const char * pcClass,
		     const char * pcConstr,
		     ...)
{
  va_list ap;
  jclass jcObject;
  jmethodID jmObject;

  va_start(ap, pcConstr);

  /* Get the object's class */
  if ((jcObject= (*env)->FindClass(env, pcClass)) == NULL)
    return NULL;

  /* Get the constructor's method ID */
  if ((jmObject= (*env)->GetMethodID(env, jcObject,
				     "<init>", pcConstr)) == NULL)
    return NULL;

  /* Build new object... */
  return (*env)->NewObjectV(env, jcObject, jmObject, ap);
}

// -----[ cbgp_jni_get_object_method ]-------------------------------
/**
 * Get the methodID of an object's method.
 */
jmethodID cbgp_jni_get_object_method(JNIEnv * jEnv, jobject joObject,
				     const char * pcMethod,
				     const char * pcSignature)
{
  jclass jcObject;
  jmethodID jmObject;

  /* Get the object's class */
  if ((jcObject= (*jEnv)->GetObjectClass(jEnv, joObject)) == NULL)
    return NULL;

  /* Get the method */
  if ((jmObject= (*jEnv)->GetMethodID(jEnv, jcObject, pcMethod,
				     pcSignature)) == NULL)
    return NULL;

  return jmObject;
}

// -----[ cbgp_jni_call_void ]---------------------------------------
/**
 * Call a void method of the given object.
 */
int cbgp_jni_call_void(JNIEnv * jEnv, jobject joObject,
		       const char * pcMethod,
		       const char * pcSignature, ...)
{
  va_list ap;
  jmethodID jmObject;

  va_start(ap, pcSignature);

  if ((jmObject= cbgp_jni_get_object_method(jEnv, joObject, pcMethod,
					    pcSignature)) == NULL)
    return -1;

  /* Call the method */
  (*jEnv)->CallVoidMethodV(jEnv, joObject, jmObject, ap);

  return 0;
}

// -----[ cbgp_jni_call_boolean ]------------------------------------
/**
 * Call a boolean method of the given object.
 */
int cbgp_jni_call_boolean(JNIEnv * jEnv, jobject joObject,
			  const char * pcMethod,
			  const char * pcSignature, ...)
{
  va_list ap;
  jclass jcObject;
  jmethodID jmObject;

  va_start(ap, pcSignature);

  /* Get the object's class */
  if ((jcObject= (*jEnv)->GetObjectClass(jEnv, joObject)) == NULL)
    return -1;

  /* Get the method */
  if ((jmObject= (*jEnv)->GetMethodID(jEnv, jcObject, pcMethod,
				     pcSignature)) == NULL)
    return -1;

  /* Call the method */
  return (*jEnv)->CallBooleanMethodV(jEnv, joObject, jmObject, ap);
}

// -----[ cbgp_jni_call_Object ]-------------------------------------
/**
 * Call an Object method of the given object.
 */
jobject cbgp_jni_call_Object(JNIEnv * jEnv, jobject joObject,
			 const char * pcMethod,
			 const char * pcSignature, ...)
{
  va_list ap;
  jmethodID jmObject;
  jobject joReturnObject;

  va_start(ap, pcSignature);

  if ((jmObject= cbgp_jni_get_object_method(jEnv, joObject, pcMethod,
					    pcSignature)) == NULL)
    return NULL;

  /* Call the method */
  joReturnObject= (*jEnv)->CallObjectMethodV(jEnv, joObject, jmObject, ap);

  return joReturnObject;
}


/////////////////////////////////////////////////////////////////////
// Management of standard Java API objects
/////////////////////////////////////////////////////////////////////

// -----[ cbgp_jni_new_Short ]---------------------------------------
jobject cbgp_jni_new_Short(JNIEnv * jEnv, jshort jsValue)
{
  return cbgp_jni_new(jEnv, "java/lang/Short", "(S)V", jsValue);
}

// -----[ cbgp_jni_new_String ]--------------------------------------
/**
 * This function returns a jstring object from a char * pointer.
 */
jstring cbgp_jni_new_String(JNIEnv * jEnv, const char * pcString)
{
  jstring jsString= NULL;

  if (pcString != NULL)
    jsString= (*jEnv)->NewStringUTF(jEnv, pcString);

  return jsString;
}

// -----[ cbgp_jni_new_Vector ]--------------------------------------
jobject cbgp_jni_new_Vector(JNIEnv * jEnv)
{
  return cbgp_jni_new(jEnv, "java/util/Vector", "()V");
}

// -----[ cbgp_jni_Vector_add ]--------------------------------------
int cbgp_jni_Vector_add(JNIEnv * jEnv, jobject joVector, jobject joObject)
{
  return cbgp_jni_call_void(jEnv, joVector, "addElement",
			    "(Ljava/lang/Object;)V", joObject);
}

/*
// -----[ cbgp_jni_new_HashMap ]-------------------------------------
jobject cbgp_jni_new_HashMap(JNIEnv * jEnv)
{
  return cbgp_jni_new(jEnv, "java/util/HashMap", "()V");
}

// -----[ cbgp_jni_HashMap_put ]-------------------------------------
jobject cbgp_jni_HashMap_put(JNIEnv * jEnv, jobject joHashMap,
			     jobject joKey, jobject joValue)
{
  return cbgp_jni_call_Object(jEnv, joHashMap, "put",
			      "(Ljava/lang/Object;Ljava/lang/Object;)"
			      "Ljava/lang/Object;",
			      joKey, joValue);
}
*/

// -----[ cbgp_jni_new_Hashtable ]-----------------------------------
jobject cbgp_jni_new_Hashtable(JNIEnv * jEnv)
{
  return cbgp_jni_new(jEnv, "java/util/Hashtable", "()V");
}

// -----[ cbgp_jni_Hashtable_put ]-----------------------------------
jobject cbgp_jni_Hashtable_put(JNIEnv * jEnv, jobject joHashtable,
			       jobject joKey, jobject joValue)
{
  return cbgp_jni_call_Object(jEnv, joHashtable, "put",
			      "(Ljava/lang/Object;Ljava/lang/Object;)"
			      "Ljava/lang/Object;",
			      joKey, joValue);
}

// -----[ cbgp_jni_new_ArrayList ]-----------------------------------
jobject cbgp_jni_new_ArrayList(JNIEnv * jEnv)
{
  return cbgp_jni_new(jEnv, "java/util/ArrayList", "()V");
}

// -----[ cbgp_jni_ArrayList_add ]-----------------------------------
int cbgp_jni_ArrayList_add(JNIEnv * jEnv, jobject joArrayList,
			   jobject joItem)
{
  return cbgp_jni_call_boolean(jEnv, joArrayList, "add",
			       "(Ljava/lang/Object;)Z",
			       joItem);
}

// -----[ jni_Object_hashCode ]--------------------------------------
jint jni_Object_hashCode(JNIEnv * jEnv, jobject joObject)
{
  jclass jcObject= (*jEnv)->GetObjectClass(jEnv, joObject);
  jmethodID jmObject= (*jEnv)->GetMethodID(jEnv, jcObject, "hashCode", "()I");
  return (*jEnv)->CallIntMethod(jEnv, joObject, jmObject);
}

