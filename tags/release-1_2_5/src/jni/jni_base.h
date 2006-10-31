// ==================================================================
// @(#)jni_base.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/03/2006
// @lastdate 27/03/2006
// ==================================================================

#ifndef __JNI_BASE_H__
#define __JNI_BASE_H__

#include <jni.h>

/////////////////////////////////////////////////////////////////////
// Helper functions for Java objects creation and method calls
/////////////////////////////////////////////////////////////////////

// -----[ cbgp_jni_new ]---------------------------------------------
extern jobject cbgp_jni_new(JNIEnv * env, const char * pcClass,
			    const char * pcConstr, ...);
// -----[ cbgp_jni_call_void ]---------------------------------------
extern int cbgp_jni_call_void(JNIEnv * env, jobject joObject,
			      const char * pcMethod,
			      const char * pcSignature, ...);
// -----[ cbgp_jni_call_boolean ]------------------------------------
extern int cbgp_jni_call_boolean(JNIEnv * env, jobject joObject,
				 const char * pcMethod,
				 const char * pcSignature, ...);
// -----[ cbgp_jni_call_Object ]---------------------------------------
extern jobject cbgp_jni_call_Object(JNIEnv * env, jobject joObject,
				    const char * pcMethod,
				    const char * pcSignature, ...);


/////////////////////////////////////////////////////////////////////
// Management of standard Java API objects
/////////////////////////////////////////////////////////////////////

// -----[ cbgp_jni_new_Short ]---------------------------------------
extern jobject cbgp_jni_new_Short(JNIEnv * jEnv, jshort jsValue);
// -----[ cbgp_jni_new_String ]--------------------------------------
extern jstring cbgp_jni_new_String(JNIEnv * jEnv, const char * pcString);
// -----[ cbgp_jni_new_Vector ]--------------------------------------
extern jobject cbgp_jni_new_Vector(JNIEnv * jEnv);
// -----[ cbgp_jni_Vector_add ]--------------------------------------
extern int cbgp_jni_Vector_add(JNIEnv * jEnv, jobject joVector,
			       jobject joObject);
/*
// -----[ cbgp_jni_new_HashMap ]-------------------------------------
extern jobject cbgp_jni_new_HashMap(JNIEnv * jEnv);
// -----[ cbgp_jni_HashMap_put ]-------------------------------------
extern jobject cbgp_jni_HashMap_put(JNIEnv * jEnv, jobject joHashMap,
				    jobject joKey, jobject joValue);
*/
// -----[ cbgp_jni_new_Hashtable ]-----------------------------------
extern jobject cbgp_jni_new_Hashtable(JNIEnv * jEnv);
// -----[ cbgp_jni_Hashtable_put ]-----------------------------------
extern jobject cbgp_jni_Hashtable_put(JNIEnv * jEnv, jobject joHashMap,
				      jobject joKey, jobject joValue);
// -----[ cbgp_jni_new_ArrayList ]-----------------------------------
extern jobject cbgp_jni_new_ArrayList(JNIEnv * jEnv);
// -----[ cbgp_jni_ArrayList_add ]-----------------------------------
extern int cbgp_jni_ArrayList_add(JNIEnv * jEnv, jobject joArrayList,
				  jobject joItem);

// -----[ jni_Object_hashCode ]--------------------------------------
extern jint jni_Object_hashCode(JNIEnv * jEnv, jobject joObject);

#endif /* __JNI_BASE_H__ */
