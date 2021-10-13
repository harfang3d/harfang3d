// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <jni.h>

struct android_app;

namespace gs {

/// Return the JNI environment used to initialize the application class loader.
JNIEnv *GetClassLoaderEnv();

/// Initialize the application class loader.
void InitializeClassLoader(JNIEnv *env, jobject native_activity_clazz);
/// Find a Java class using the application class loader object.
jclass FindJavaClass(const char *name);

/// Return the native android application object.
android_app *GetAndroidApp();

} // gs
