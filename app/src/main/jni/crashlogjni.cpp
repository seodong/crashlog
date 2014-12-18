#include <jni.h>

#include "sig_segv.h"

static JavaVM* g_jvm = NULL;

char* gExternalStoragePath = NULL;
char gAppVersion[32];

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* ajvm, void* reserved) {
	g_jvm = ajvm;

	return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
	free(gExternalStoragePath);
}

JNIEXPORT void JNICALL Java_com_example_crashlog_CrashlogExampleJni_setLogPath(
	JNIEnv* env,
	jobject thiz,
	jstring jpath)
{
	//
	const char* path = env->GetStringUTFChars(jpath, NULL);
	if (!path)
		return;

	if (gExternalStoragePath != NULL)
		free(gExternalStoragePath);

	int len = strlen(path) + 1;
	gExternalStoragePath = (char*) malloc(len);
	strcpy(gExternalStoragePath, path);
	gExternalStoragePath[len - 1] = '\0';

	//
	setup_sigsegv();
}

JNIEXPORT void JNICALL Java_com_example_crashlog_CrashlogExampleJni_setAppVersion(
	JNIEnv* env,
	jobject thiz,
	jstring jver)
{
	const char* ver = env->GetStringUTFChars(jver, NULL);

	memset(gAppVersion, 0, sizeof(gAppVersion));
	strncpy(gAppVersion, ver, 31);
}

JNIEXPORT void JNICALL Java_com_example_crashlog_CrashlogExampleJni_forceCrash(
	JNIEnv* env,
	jobject thiz)
{
	int* p = NULL;
	*p = 1;
}

#ifdef __cplusplus
}
#endif

