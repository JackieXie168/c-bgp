// ==================================================================
// @(#)listener.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/06/2007
// $Id: listener.c,v 1.5 2008-05-20 12:11:38 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni/exceptions.h>
#include <jni/listener.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

// -----[ jni_listener_init ]----------------------------------------
void jni_listener_init(SJNIListener * pListener)
{
  pListener->jVM= NULL;
  pListener->joListener= NULL;
}

// -----[ jni_listener_set ]-----------------------------------------
void jni_listener_set(SJNIListener * pListener, JNIEnv * jEnv,
		      jobject joListener)
{
  jni_listener_unset(pListener, jEnv);

  if (joListener == NULL)
    return;

  // Get reference to Java Virtual Machine (required by the JNI callback)
  if (pListener->jVM == NULL)
    if ((*jEnv)->GetJavaVM(jEnv, &pListener->jVM) != JNI_OK) {
      throw_CBGPException(jEnv, "could not get reference to Java VM");
      return;
    }

  // Add a global reference to the listener object (returns NULL if
  // system has run out of memory)
  pListener->joListener= (*jEnv)->NewGlobalRef(jEnv, joListener);
  if (pListener->joListener == NULL)
    jni_abort(jEnv, "Could not obtain global reference in jni_set_listener()");
}

// -----[ jni_listener_unset ]---------------------------------------
void jni_listener_unset(SJNIListener * pListener, JNIEnv * jEnv)
{
  // Remove global reference to former listener (if applicable)
  if (pListener->joListener != NULL)
    (*jEnv)->DeleteGlobalRef(jEnv, pListener->joListener);
  pListener->joListener= NULL;
}
