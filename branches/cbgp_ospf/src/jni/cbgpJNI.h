/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class cbgpJNI */

#ifndef _Included_cbgpJNI
#define _Included_cbgpJNI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     cbgpJNI
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_init
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_finalize
  (JNIEnv *, jobject);

/*
 * Class:     cbgpJNI
 * Method:    nodeAdd
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeAdd
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    nodeLinkAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeLinkAdd
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     cbgpJNI
 * Method:    nodeLinkWeight
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeLinkWeight
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     cbgpJNI
 * Method:    nodeLinkUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeLinkUp
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    nodeLinkDown
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeLinkDown
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    nodeRouteAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeRouteAdd
  (JNIEnv *, jobject, jstring, jstring, jstring, jint);

/*
 * Class:     cbgpJNI
 * Method:    nodeSpfPrefix
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeSpfPrefix
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    nodeInterfaceAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_nodeInterfaceAdd
  (JNIEnv *, jobject, jstring, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    nodeShowLinks
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_nodeShowLinks
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    nodeShowRT
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cbgpJNI_nodeShowRT
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpRouterAdd
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterNetworkAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpRouterNetworkAdd
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterNeighborAdd
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpRouterNeighborAdd
  (JNIEnv *, jobject, jstring, jstring, jint);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterNeighborNextHopSelf
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_bgpRouterNeighborNextHopSelf
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterNeighborUp
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpRouterNeighborUp
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterRescan
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpRouterRescan
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterShowRib
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cbgpJNI_bgpRouterShowRib
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpRouterShowRibIn
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_bgpRouterShowRibIn
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterInit
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpFilterInit
  (JNIEnv *, jobject, jstring, jstring, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterMatchPrefixIn
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpFilterMatchPrefixIn
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterMatchPrefixIs
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpFilterMatchPrefixIs
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterAction
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_bgpFilterAction
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     cbgpJNI
 * Method:    bgpFilterFinalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_bgpFilterFinalize
  (JNIEnv *, jobject);

/*
 * Class:     cbgpJNI
 * Method:    simRun
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cbgpJNI_simRun
  (JNIEnv *, jobject);

/*
 * Class:     cbgpJNI
 * Method:    simPrint
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_simPrint
  (JNIEnv *, jobject, jstring);

/*
 * Class:     cbgpJNI
 * Method:    runCmd
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cbgpJNI_runCmd
  (JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif
