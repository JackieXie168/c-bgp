// ==================================================================
// @(#)jni_base.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/03/2006
// @lastdate 01/06/2007
// ==================================================================

#ifndef __JNI_BASE_H__
#define __JNI_BASE_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // Helper functions for Java objects creation and method calls
  ///////////////////////////////////////////////////////////////////

  // -----[ cbgp_jni_new ]-------------------------------------------
  jobject cbgp_jni_new(JNIEnv * env, const char * pcClass,
		       const char * pcConstr, ...);
  // -----[ cbgp_jni_call_void ]-------------------------------------
  int cbgp_jni_call_void(JNIEnv * env, jobject joObject,
			 const char * pcMethod,
			 const char * pcSignature, ...);
  // -----[ cbgp_jni_call_boolean ]----------------------------------
  int cbgp_jni_call_boolean(JNIEnv * env, jobject joObject,
			    const char * pcMethod,
			    const char * pcSignature, ...);
  // -----[ cbgp_jni_call_Object ]-----------------------------------
  jobject cbgp_jni_call_Object(JNIEnv * env, jobject joObject,
			       const char * pcMethod,
			       const char * pcSignature, ...);
  
  
  ///////////////////////////////////////////////////////////////////
  // Management of standard Java API objects
  ///////////////////////////////////////////////////////////////////
  
  // -----[ cbgp_jni_new_Short ]-------------------------------------
  jobject cbgp_jni_new_Short(JNIEnv * jEnv, jshort jsValue);
  // -----[ cbgp_jni_new_String ]------------------------------------
  jstring cbgp_jni_new_String(JNIEnv * jEnv, const char * pcString);
  // -----[ cbgp_jni_new_Vector ]------------------------------------
  jobject cbgp_jni_new_Vector(JNIEnv * jEnv);
  // -----[ cbgp_jni_Vector_add ]------------------------------------
  int cbgp_jni_Vector_add(JNIEnv * jEnv, jobject joVector,
			  jobject joObject);
  /*
  // -----[ cbgp_jni_new_HashMap ]-----------------------------------
  jobject cbgp_jni_new_HashMap(JNIEnv * jEnv);
  // -----[ cbgp_jni_HashMap_put ]-----------------------------------
  jobject cbgp_jni_HashMap_put(JNIEnv * jEnv, jobject joHashMap,
  jobject joKey, jobject joValue);
  */
  // -----[ cbgp_jni_new_Hashtable ]---------------------------------
  jobject cbgp_jni_new_Hashtable(JNIEnv * jEnv);
  // -----[ cbgp_jni_Hashtable_put ]---------------------------------
  jobject cbgp_jni_Hashtable_put(JNIEnv * jEnv, jobject joHashMap,
				 jobject joKey, jobject joValue);
  // -----[ cbgp_jni_new_ArrayList ]---------------------------------
  jobject cbgp_jni_new_ArrayList(JNIEnv * jEnv);
  // -----[ cbgp_jni_ArrayList_add ]---------------------------------
  int cbgp_jni_ArrayList_add(JNIEnv * jEnv, jobject joArrayList,
			     jobject joItem);
  
  // -----[ jni_Object_hashCode ]------------------------------------
  jint jni_Object_hashCode(JNIEnv * jEnv, jobject joObject);
  
#ifdef __cplusplus
}
#endif

#endif /* __JNI_BASE_H__ */
