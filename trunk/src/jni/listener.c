// ==================================================================
// @(#)listener.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/06/2007
// $Id: listener.c,v 1.6 2009-03-26 13:24:14 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni/exceptions.h>
#include <jni/listener.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

// -----[ jni_listener_init ]----------------------------------------
void jni_listener_init(jni_listener_t * listener)
{
  listener->jVM= NULL;
  listener->joListener= NULL;
}

// -----[ jni_listener_set ]-----------------------------------------
void jni_listener_set(jni_listener_t * listener, JNIEnv * env,
		      jobject java_listener)
{
  jni_listener_unset(listener, env);

  if (java_listener == NULL)
    return;

  // Get reference to Java Virtual Machine (required by the JNI callback)
  if (listener->jVM == NULL)
    if ((*env)->GetJavaVM(env, &listener->jVM) != JNI_OK) {
      throw_CBGPException(env, "could not get reference to Java VM");
      return;
    }

  // Add a global reference to the listener object (returns NULL if
  // system has run out of memory)
  listener->joListener= (*env)->NewGlobalRef(env, java_listener);
  if (listener->joListener == NULL)
    jni_abort(env, "Could not obtain global reference in jni_set_listener()");
}

// -----[ jni_listener_unset ]---------------------------------------
void jni_listener_unset(jni_listener_t * listener, JNIEnv * env)
{
  // Remove global reference to former listener (if applicable)
  if (listener->joListener != NULL)
    (*env)->DeleteGlobalRef(env, listener->joListener);
  listener->joListener= NULL;
}

// -----[ jni_listener_get_env ]-----------------------------------
void jni_listener_get_env(jni_listener_t * listener, JNIEnv ** env_ref)
{
  JavaVM * java_vm= listener->jVM;
  void ** env_ref_as_void= (void **) &(*env_ref);
  
  /* Attach the current thread to the Java VM (and get a pointer to
   * the JNI environment). This is required since the calling thread
   * might be different from the one that initialized the related
   * listener object. */
  if ((*java_vm)->AttachCurrentThread(java_vm, env_ref_as_void, NULL) != 0)
    jni_abort(*env_ref, "AttachCurrentThread failed in JNI\n");
}
