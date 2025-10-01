#include <launcher-utils/jni.hpp>
#include <launcher-utils/geode.hpp>

#include <string_view>
#include <string>

using namespace launcher_utils;

geode::Result<JNIEnv*> jni::getEnv() {
	static thread_local JNIEnv* env = nullptr;
	if (env) {
		return geode::Ok(env);
	}

	auto vm = cocos2d::JniHelper::getJavaVM();

	auto ret = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4);
	switch (ret) {
		case JNI_OK:
			return geode::Ok(env);
		case JNI_EDETACHED:
			env = nullptr;
			return geode::Err("getEnv: environment is on a separate thread");
		default:
			env = nullptr;
			return geode::Err(fmt::format("getEnv: {}", ret));
	}
}

geode::Result<> jni::checkForExceptions(JNIEnv* env) {
	if (env->ExceptionCheck() == JNI_TRUE) {
		auto e = env->ExceptionOccurred();
		env->ExceptionClear();

		GEODE_UNWRAP_INTO(auto msg, callMethod<std::string>(env, "java/lang/Throwable", "getMessage", "()Ljava/lang/String;", e));

		geode::Err(msg);
	}

	return geode::Ok();
}

geode::Result<jni::GlobalRef&> jni::getClassId(JNIEnv* env, const char* className) {
	static std::unordered_map<std::string, jni::GlobalRef> s_classCache;

	if (auto it = s_classCache.find(className); it != s_classCache.end()) {
		return geode::Ok(it->second);
	}

	auto classId = LocalRef(env->FindClass(className));
	if (!classId) {
		env->ExceptionClear();
		return geode::Err(fmt::format("Failed to find class {}", className));
	}

	auto& classRef = s_classCache.try_emplace(className, classId.get<jclass>()).first->second;

	return geode::Ok(classRef);
}

geode::Result<jni::MethodInfo&> jni::getStaticMethodInfo(JNIEnv* env, const char* className, const char* methodName, const char* paramSignature) {
	static std::unordered_map<std::string, MethodInfo> s_methodCache;

	auto methodSignature = fmt::format("{}.{}{}", className, methodName, paramSignature);
	if (auto it = s_methodCache.find(methodSignature); it != s_methodCache.end()) {
		return geode::Ok(it->second);
	}

	GEODE_UNWRAP_INTO(auto& classId, getClassId(env, className));

	auto methodId = env->GetStaticMethodID(classId.get<jclass>(), methodName, paramSignature);
	if (!methodId) {
		env->ExceptionClear();
		return geode::Err(fmt::format("Failed to find static method {}.{}{}", className, methodName, paramSignature));
	}

	auto& methodInfo = s_methodCache.try_emplace(
		methodSignature,
		classId,
		methodId
	).first->second;

	return geode::Ok(methodInfo);
}

geode::Result<jni::MethodInfo&> jni::getMethodInfo(JNIEnv* env, const char* className, const char* methodName, const char* paramSignature) {
	static std::unordered_map<std::string, jni::MethodInfo> s_methodCache;

	auto methodSignature = fmt::format("{}.{}{}", className, methodName, paramSignature);
	if (auto it = s_methodCache.find(methodSignature); it != s_methodCache.end()) {
		return geode::Ok(it->second);
	}

	GEODE_UNWRAP_INTO(auto& classId, getClassId(env, className));

	auto methodId = env->GetMethodID(classId.get<jclass>(), methodName, paramSignature);
	if (!methodId) {
		env->ExceptionClear();
		return geode::Err(fmt::format("Failed to find static method {}.{}{}", className, methodName, paramSignature));
	}

	return geode::Ok(s_methodCache.try_emplace(
		methodSignature,
		classId,
		methodId
	).first->second);
}

jni::LocalRef jni::toJavaArray(JNIEnv* env, std::span<std::int64_t> arr) {
	auto ptr = env->NewLongArray(arr.size());
	env->SetLongArrayRegion(ptr, 0, arr.size(), arr.data());

	return LocalRef(ptr);
}

geode::Result<std::vector<int>> jni::extractArray(JNIEnv* env, jintArray array) {
	if (array == nullptr) {
		return geode::Err("extractArray: null array");
	}

	auto len = env->GetArrayLength(array);
	std::vector<int> r(len);
	env->GetIntArrayRegion(array, 0, len, r.data());
	return geode::Ok(r);
}

geode::Result<std::string> jni::toString(JNIEnv* env, jstring string) {
	if (!string) {
		return geode::Err("convertString: null string");
	}

	auto chars = env->GetStringChars(string, nullptr);
	if (!chars) {
		return geode::Err("convertString: GetStringChars failed");
	}

	std::size_t length = env->GetStringLength(string);
	std::u16string_view strData{
		reinterpret_cast<const char16_t*>(chars),
		length
	};

	auto r = geode::utils::string::utf16ToUtf8(strData);

	env->ReleaseStringChars(string, chars);

	return r;
}

geode::Result<jni::LocalRef> jni::toJString(JNIEnv* env, std::string_view string) {
	GEODE_UNWRAP_INTO(auto wString, geode::utils::string::utf8ToUtf16(string));

	auto jString = env->NewString(
		reinterpret_cast<const jchar*>(wString.data()),
		wString.size()
	);
	if (!jString) {
		return geode::Err("toJavaString: NewString returned null");
	}

	return geode::Ok(LocalRef(jString));
}

geode::Result<int> launcher_utils::getConnectedControllerCount() {
	return jni::callStaticMethod<int>("com/geode/launcher/utils/GeodeUtils", "controllersConnected", "()I");
}

geode::Result<std::vector<int>> launcher_utils::getConnectedDevices() {
	return jni::callStaticMethod<std::vector<int>>("com/geode/launcher/utils/GeodeUtils", "getConnectedDevices", "()[I");
}

geode::Result<bool> launcher_utils::vibrateSupported() {
	return jni::callStaticMethod<bool>("com/geode/launcher/utils/GeodeUtils", "vibrateSupported", "()Z");
}

geode::Result<> launcher_utils::vibrate(std::int64_t ms) {
	return jni::callStaticMethod<void>("com/geode/launcher/utils/GeodeUtils", "vibrate", "(J)V", ms);
}

geode::Result<> launcher_utils::vibratePattern(std::span<std::int64_t> pattern, int repeat) {
	GEODE_UNWRAP_INTO(auto env, jni::getEnv());

	auto arr = jni::toJavaArray(env, pattern);
	auto r = jni::callStaticMethod<void>(env, "com/geode/launcher/utils/GeodeUtils", "vibratePattern", "([JI)V", *arr, repeat);

	return r;
}
