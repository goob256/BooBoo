#include "shim4/cloud.h"
#include "shim4/util.h"

#if defined IOS || defined MAS
#include "shim4/apple.h"
#endif

using namespace noo;

#if defined GOOGLE_PLAY || defined AMAZON
#include <jni.h>

Sint64 cloud_date_android(std::string filename)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "cloud_date", "(Ljava/lang/String;)J");

	jstring S = env->NewStringUTF(filename.c_str());

	Sint64 ret;
	if (method_id != 0) {
		ret = env->CallLongMethod(activity, method_id, S);
	}
	else {
		ret = -1;
	}

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}

bool cloud_delete_android(std::string filename)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "cloud_delete", "(Ljava/lang/String;)Z");

	jstring S = env->NewStringUTF(filename.c_str());

	bool ret;
	if (method_id != 0) {
		ret = env->CallBooleanMethod(activity, method_id, S);
	}
	else {
		ret = false;
	}

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}

char *cloud_read_android(std::string filename, int *sz)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(filename.c_str());

	jmethodID method_id = env->GetMethodID(clazz, "cloud_read", "(Ljava/lang/String;)[B");

	char *ret;
	if (method_id != 0) {
		jbyteArray b = (jbyteArray)env->CallObjectMethod(activity, method_id, S);
		if (b == 0) {
			ret = 0;
		}
		else {
			*sz = env->GetArrayLength(b);
			ret = new char[*sz];
			env->GetByteArrayRegion(b, 0, *sz, (jbyte *)ret);
			env->DeleteLocalRef(b);
		}
	}
	else {
		ret = 0;
	}

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}

bool cloud_save_android(std::string filename, const char *bytes, int sz)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(filename.c_str());
	jbyteArray b = env->NewByteArray(sz);
	env->SetByteArrayRegion(b, 0, sz, (const jbyte *)bytes);

	jmethodID method_id = env->GetMethodID(clazz, "cloud_save", "(Ljava/lang/String;[B)Z");

	bool ret;
	if (method_id != 0) {
		ret = env->CallBooleanMethod(activity, method_id, S, b);
	}
	else {
		ret = false;
	}

	env->DeleteLocalRef(b);
	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}

void cloud_synchronise_android()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "cloud_synchronise", "()V");

	if (method_id != 0) {
		env->CallVoidMethod(activity, method_id);
	}

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}

int cloud_get_error_code_android()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "cloud_get_error_code", "()I");

	int ret;
	if (method_id != 0) {
		ret = env->CallIntMethod(activity, method_id);
	}
	else {
		ret = 0xffffffff;
	}

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}

/*
bool cloud_exists_android(std::string filename)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "cloud_exists", "(Ljava/lang/String;)Z");

	jstring S = env->NewStringUTF(filename.c_str());

	bool ret;
	if (method_id != 0) {
		ret = env->CallBooleanMethod(activity, method_id, S);
	}
	else {
		ret = false;
	}

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}

bool cloud_mkdir_android(std::string filename)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "cloud_mkdir", "(Ljava/lang/String;)Z");

	jstring S = env->NewStringUTF(filename.c_str());

	bool ret;
	if (method_id != 0) {
		ret = env->CallBooleanMethod(activity, method_id, S);
	}
	else {
		ret = false;
	}

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return ret;
}
*/
#endif

namespace noo {

namespace util {

Sint64 cloud_date(std::string filename)
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_date_android(filename);
#elif defined IOS || defined MAS
	return apple_cloud_date(filename);
#else
	return -1;
#endif
}

bool cloud_delete(std::string filename)
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_delete_android(filename);
#elif defined IOS || defined MAS
	return apple_cloud_delete(filename);
#else
	return false;
#endif
}

char *cloud_read(std::string filename, int *sz)
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_read_android(filename, sz);
#elif defined IOS || defined MAS
	return apple_cloud_read(filename, sz);
#else
	return 0;
#endif
}

bool cloud_save(std::string filename, const char *bytes, int sz)
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_save_android(filename, bytes, sz);
#elif defined IOS || defined MAS
	return apple_cloud_save(filename, bytes, sz);
#else
	return false;
#endif
}

int cloud_get_error_code()
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_get_error_code_android();
#elif defined IOS || defined MAS
	return apple_cloud_get_error_code();
#else
	return -1;
#endif
}

void cloud_synchronise()
{
#if defined GOOGLE_PLAY || defined AMAZON
	cloud_synchronise_android();
#elif defined IOS || defined MAS
	apple_cloud_synchronise();
#endif
}

/*
bool cloud_exists(std::string filename)
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_exists_android(filename);
#else
	return false;
#endif
}

bool cloud_mkdir(std::string filename)
{
#if defined GOOGLE_PLAY || defined AMAZON
	return cloud_mkdir_android(filename);
#else
	return false;
#endif
}
*/

} // End namespace util

} // End namespace noo
