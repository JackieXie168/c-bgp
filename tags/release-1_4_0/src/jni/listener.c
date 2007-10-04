// ==================================================================
// @(#)listener.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/06/2007
// @lastdate 29/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <jni/listener.h>
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

  // Get reference to Java Virtual Machine (required by the JNI callback)
  if (pListener->jVM == NULL)
    if ((*jEnv)->GetJavaVM(jEnv, &pListener->jVM) != 0) {
      cbgp_jni_throw_CBGPException(jEnv, "could not get reference to Java VM");
      return;
    }

  // Add a global reference to the listener object (returns NULL if
  // system has run out of memory)
  pListener->joListener= (*jEnv)->NewGlobalRef(jEnv, joListener);
  if (pListener->joListener == NULL) {
    fprintf(stderr, "ERROR: NewGlobalRef returned NULL in consoleSetOutListener");
    return;
  }
}

// -----[ jni_listener_unset ]---------------------------------------
void jni_listener_unset(SJNIListener * pListener, JNIEnv * jEnv)
{
  // Remove global reference to former listener (if applicable)
  if (pListener->joListener != NULL)
    (*jEnv)->DeleteGlobalRef(jEnv, pListener->joListener);
  pListener->joListener= NULL;
}
