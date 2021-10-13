// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <android_native_app_glue.h>
#include "platform/android/logcat_immediate_output.h"
#include "platform/android/input_system.h"
#include "platform/android/android_jni.h"
#include "platform/android/io_aasset.h"
#include "platform/android/jnipp.h"
#include "platform/input_system.h"
#include "foundation/io_cfile.h"

namespace hg {

bool InitPlatform() {
	g_log.get().immediate_log_output = &logcat_immediate_output;
	RegisterInputDevices(g_input_system.get());
	return true;
}

std::string GetPlatformLocale() {
	using namespace jnipp;

	Env::Scope scope(GetClassLoaderEnv());
	auto default_locale = StaticMethod<Object>("java/util/Locale", "getDefault", "()Ljava/util/Locale;").call();
	auto locale = Method<String>("java/util/Locale", "toLanguageTag", "()Ljava/lang/String;").call(default_locale);
//	auto language = Method<String>("java/util/Locale", "getLanguage", "()Ljava/lang/String;").call(default_locale);

//	return static_cast<String>(language).std_str();
	return static_cast<String>(locale).std_str();
}

std::string GetTempFilename(const char *prefix) {
	char tmpl[1024 + 1];
	snprintf(tmpl, 1024, "%s_XXXXXX", prefix);
	mktemp(tmpl);
	return tmpl;
}

io::sDriver NewExternalFilesDriver() {
	debug(format("Returning new external files driver mapped to %1").arg(GetAndroidApp()->activity->externalDataPath));
	return std::make_shared<io::CFile>(GetAndroidApp()->activity->externalDataPath, true);
}

io::sDriver NewAppDataDriver() { return std::make_shared<io::AAssetDriver>(GetAndroidApp()->activity->assetManager); }

} // hg
