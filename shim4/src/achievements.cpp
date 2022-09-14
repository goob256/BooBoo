#include "shim4/achievements.h"

#if defined IOS || defined MAS
#include "shim4/gamecenter.h"
#endif

#ifdef STEAMWORKS
#include "shim4/steamworks.h"
#endif

using namespace noo;

namespace noo {

namespace util {

#if defined GOOGLE_PLAY || defined AMAZON
#include <jni.h>

bool achieve_android(char *id)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(id);

	jmethodID method_id = env->GetMethodID(clazz, "achieve", "(Ljava/lang/String;)V");

	bool ret;
	if (method_id != 0) {
		env->CallVoidMethod(activity, method_id, S);
		ret = true;
	}
	else {
		ret = false;
	}
	
	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;

	return true;
}

bool show_achievements_android()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "show_achievements", "()Z");
	
	bool ret;
	if (method_id != 0) {
		ret = env->CallBooleanMethod(activity, method_id);
	}
	else {
		ret = false;
	}

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}
#endif

bool achieve(void *id)
{
#if defined GOOGLE_PLAY || defined AMAZON
	// Hack to convert void * to int
	//return achieve_android((int *)id - (int *)0);
	return achieve_android((char *)id);
#elif defined IOS || defined MAS
	return achieve_gamecenter((char *)id);
#elif defined STEAMWORKS
	return achieve_steam((char *)id);
#else
	return false; // failed
#endif
}

bool show_achievements()
{
#if defined GOOGLE_PLAY || defined AMAZON
	return show_achievements_android();
#elif defined IOS || defined MAS
	return show_achievements_gamecenter();
#else
	return false; // failed
#endif
}

} // End namespace util

} // End namespace noo
