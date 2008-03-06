// ==================================================================
// @(#)jni_base.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/03/2006
// @lastdate 03/03/2008
// ==================================================================

#ifndef __JNI_BASE_H__
#define __JNI_BASE_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ jni_abort ]----------------------------------------------
  void jni_abort(JNIEnv * jEnv, char * pcMsg, ...);

  ///////////////////////////////////////////////////////////////////
  // Helper functions for Java objects creation and method calls
  ///////////////////////////////////////////////////////////////////

  // -----[ jni_check_null ]-----------------------------------------
  int jni_check_null(JNIEnv * jEnv, jobject joObject);
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
  // -----[ cbgp_jni_new_Integer ]-----------------------------------
  jobject cbgp_jni_new_Integer(JNIEnv * jEnv, jint jiValue);
  // -----[ cbgp_jni_new_Long ]--------------------------------------
  jobject cbgp_jni_new_Long(JNIEnv * jEnv, jlong jlValue);
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
  // -----[ cbgp_jni_Hashtable_get ]---------------------------------
  jobject cbgp_jni_Hashtable_get(JNIEnv * jEnv, jobject joHashMap,
				 jobject joKey);
  // -----[ cbgp_jni_Hashtable_put ]---------------------------------
  jobject cbgp_jni_Hashtable_put(JNIEnv * jEnv, jobject joHashMap,
				 jobject joKey, jobject joValue);
  // -----[ cbgp_jni_new_ArrayList ]---------------------------------
  jobject cbgp_jni_new_ArrayList(JNIEnv * jEnv);
  // -----[ cbgp_jni_ArrayList_add ]---------------------------------
  int cbgp_jni_ArrayList_add(JNIEnv * jEnv, jobject joArrayList,
			     jobject joItem);
  
#ifdef __cplusplus
}
#endif

#endif /* __JNI_BASE_H__ */
