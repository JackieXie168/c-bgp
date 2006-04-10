/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class jni_cbgp_cbgpJNI */

#ifndef _Included_jni_cbgp_cbgpJNI
#define _Included_jni_cbgp_cbgpJNI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_init
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_finalize
  (JNIEnv *, jobject);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeAdd
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeAdd
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeLinkAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkAdd
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeLinkWeight
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkWeight
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeLinkUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkUp
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeLinkDown
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeLinkDown
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeRouteAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeRouteAdd
  (JNIEnv *, jobject, jstring, jstring, jstring, jint);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeSpfPrefix
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeSpfPrefix
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeInterfaceAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_nodeInterfaceAdd
  (JNIEnv *, jobject, jstring, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeShowLinks
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_nodeShowLinks
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    nodeShowRT
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_jni_cbgp_cbgpJNI_nodeShowRT
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterAdd
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterNetworkAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNetworkAdd
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterNeighborAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNeighborAdd
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterNeighborNextHopSelf
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNeighborNextHopSelf
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterNeighborUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterNeighborUp
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterRescan
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterRescan
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterShowRib
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterShowRib
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpRouterShowRibIn
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_bgpRouterShowRibIn
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpFilterInit
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterInit
  (JNIEnv *, jobject, jstring, jstring, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpFilterMatchPrefix
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterMatchPrefix
  (JNIEnv *, jobject, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpFilterAction
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterAction
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    bgpFilterFinalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_bgpFilterFinalize
  (JNIEnv *, jobject);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    simRun
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_jni_cbgp_cbgpJNI_simRun
  (JNIEnv *, jobject);

/*
 * Class:     jni_cbgp_cbgpJNI
 * Method:    simPrint
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_jni_cbgp_cbgpJNI_simPrint
  (JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif