// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "android_jni.h"

namespace gs {

static JNIEnv *classLoader_env = nullptr;
static jobject classLoader_cls = nullptr;
static jmethodID findClass = nullptr;

JNIEnv *GetClassLoaderEnv() { return classLoader_env; }

void InitializeClassLoader(JNIEnv *env, jobject native_activity_clazz) {
	classLoader_env = env;

	jobject nativeActivity = native_activity_clazz;
	jclass acl = env->GetObjectClass(nativeActivity);
	jmethodID getClassLoader = env->GetMethodID(acl, "getClassLoader", "()Ljava/lang/ClassLoader;");
	classLoader_cls = env->CallObjectMethod(nativeActivity, getClassLoader);

	jclass classLoader = env->FindClass("java/lang/ClassLoader");
	findClass = env->GetMethodID(classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
}

jclass FindJavaClass(const char *name) {
	jstring strClassName = classLoader_env->NewStringUTF(name);
	jclass clss = (jclass)(classLoader_env->CallObjectMethod(classLoader_cls, findClass, strClassName));
	classLoader_env->DeleteLocalRef(strClassName);
	return clss;
}

static android_app *app = nullptr;

void SetAndroidApp(android_app *app_) { app = app_; }
android_app *GetAndroidApp() { return app; }

} // gs
