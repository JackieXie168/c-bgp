// ==================================================================
// @(#)listener.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/06/2007
// $Id: listener.h,v 1.3 2009-03-26 13:24:14 bqu Exp $
// ==================================================================

/**
 * \file
 * Contain data structures and functions to manage Java listeners.
 */

#ifndef __JNI_LISTENER_H__
#define __JNI_LISTENER_H__

#include <jni.h>

// -----[ jni_listener_t ]-------------------------------------------
/**
 * Context structure used for Java callbacks (listeners)
 *
 * IMPORTANT NOTE: the joListener field must contain a global
 * reference to the Java listener !!! Use NewGlobalRef to create a
 * global reference from a local reference.
 */
typedef struct {
  JavaVM * jVM;
  jobject  joListener;
} jni_listener_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ jni_listener_init ]--------------------------------------
  /**
   * Initialize a JNI listener.
   *
   * \param listener is the JNI listener.
   */
  void jni_listener_init(jni_listener_t * listener);

  // -----[ jni_listener_set ]---------------------------------------
  /**
   * Assign a Java object to a JNI listener.
   *
   * \param listener      is the JNI listener.
   * \param env           is the JNI environment (used to get the
   *                      Java VM reference).
   * \param java_listener is the Java object listener.
   */
  void jni_listener_set(jni_listener_t * listener, JNIEnv * env,
			jobject java_listener);

  // -----[ jni_listener_unset ]-------------------------------------
  /**
   * Clear the Java object assigned to a JNI listener.
   *
   * \param listener is the JNI listener.
   * \param env      is the JNI environment.
   */
  void jni_listener_unset(jni_listener_t * listener, JNIEnv * env);

  // -----[ jni_listener_get_env ]-----------------------------------
  /**
   * Attach the current thread to the Java VM specified in the
   * listener object and get a pointer to the JNI environment.
   *
   * \param listener is the listener object.
   * \param env_ref  is a reference to the JNI environment (used to
   *                 retrieve the JNI environment).
   */
  void jni_listener_get_env(jni_listener_t * listener, JNIEnv ** env_ref);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_LISTENER_H__ */
