#pragma once

#include <Geode/Result.hpp>
#include <Geode/cocos/platform/android/jni/JniHelper.h>

#include <vector>
#include <span>

namespace launcher_utils::jni {
	/**
	 * Pulls the JNIEnv from cocos2d's JniHelper.
	 * Unlike cocos2d's JniHelper, this function will not automatically move the environment to the calling thread.
	 */
	geode::Result<JNIEnv*> getEnv();

	/**
	 * Stores a local reference to an object.
	 * This class does not create a new local reference, but will destroy the given reference once out of scope.
	 * You should not store a LocalRef for longer than a single frame. Use a GlobalRef for long-term storage.
	 */
	class LocalRef final {
		jobject m_obj{};

	public:
		LocalRef() : m_obj(nullptr) {}

		LocalRef(jobject obj) : m_obj(obj) {}

		LocalRef(const LocalRef&) = delete;
		LocalRef& operator=(const LocalRef&) = delete;

		LocalRef(LocalRef&& x) {
			std::swap(x.m_obj, m_obj);
		}

		LocalRef& operator=(LocalRef&& x) {
			std::swap(x.m_obj, m_obj);
			return *this;
		}

		explicit operator bool const() {
			return m_obj != nullptr;
		}

		jobject operator*() const {
			return m_obj;
		}

		template <typename T = jobject>
		T get() const {
			return static_cast<T>(m_obj);
		}

		~LocalRef() {
			if (m_obj) {
				if (auto env = getEnv()) {
					(*env)->DeleteLocalRef(m_obj);
				}
			}
		}
	};

	/**
	 * Stores a global reference to a jobject.
	 * When this object is copied, it will create a new global reference.
	 * To avoid this, move the object instead.
	 */
	class GlobalRef final {
		jobject m_obj{};

		void createRef(jobject obj) {
			if (obj == nullptr) {
				m_obj = nullptr;
				return;
			}

			if (auto env = getEnv()) {
				m_obj = (*env)->NewGlobalRef(obj);
			}
		}

		void freeRef() {
			if (m_obj) {
				if (auto env = getEnv()) {
					(*env)->DeleteGlobalRef(m_obj);
				}
			}

			m_obj = nullptr;
		}

	public:
		GlobalRef() : m_obj(nullptr) {}

		GlobalRef(jobject obj) {
			createRef(obj);
		}

		GlobalRef(GlobalRef&& x) {
			std::swap(m_obj, x.m_obj);
		}

		GlobalRef& operator=(GlobalRef&& x) {
			std::swap(m_obj, x.m_obj);
			return *this;
		}

		GlobalRef(const GlobalRef& x) {
			freeRef();
			createRef(x.m_obj);
		}

		GlobalRef& operator=(GlobalRef& x) {
			freeRef();
			createRef(x.m_obj);

			return *this;
		}

		~GlobalRef() {
			freeRef();
		}

		template <typename T = jobject>
		T get() const {
			return static_cast<T>(m_obj);
		}

		jobject operator*() const {
			return m_obj;
		}
	};

	/**
	 * JNI method storage helper.
	 */
	class MethodInfo final {
		GlobalRef& m_classId;
		jmethodID m_methodId;

	public:
		MethodInfo(GlobalRef& classId, jmethodID methodId)
			: m_classId(classId), m_methodId(methodId) {}

		jclass classID() const {
			return m_classId.get<jclass>();
		}

		jmethodID methodID() const {
			return m_methodId;
		}
	};

	geode::Result<> checkForExceptions(JNIEnv* env);

	/**
	 * Cached fetcher for a JNI class.
	 * className is separated by / (`java/lang/String`)
	 */
	geode::Result<GlobalRef&> getClassId(JNIEnv* env, const char* className);

	/**
	 * Cached fetcher for a static JNI method.
	 */
	geode::Result<MethodInfo&> getStaticMethodInfo(JNIEnv* env, const char* className, const char* methodName, const char* paramSignature);

	/**
	 * Cached fetcher for a non-static JNI method.
	 */
	geode::Result<MethodInfo&> getMethodInfo(JNIEnv* env, const char* className, const char* methodName, const char* paramSignature);

	/**
	 * Converts a long C array to a Java array.
	 * The returned local ref is not automatically freed.
	 * (The garbage collector should do this, if not explicitly done.)
	 */
	LocalRef toJavaArray(JNIEnv* env, std::span<std::int64_t> arr);

	/**
	 * Converts a Java integer array into an int vector.
	 */
	geode::Result<std::vector<int>> extractArray(JNIEnv* env, jintArray array);

	geode::Result<std::string> convertString(JNIEnv* env, jstring string);

	template <typename T, typename... Args> requires std::same_as<T, void>
	geode::Result<> performStaticMethodCall(JNIEnv* env, MethodInfo& info, Args... args) {
		env->CallStaticVoidMethod(info.classID(), info.methodID(), args...);
		GEODE_UNWRAP(checkForExceptions(env));
		return geode::Ok();
	}

	template <typename T, typename... Args> requires std::same_as<T, bool>
	geode::Result<bool> performStaticMethodCall(JNIEnv* env, MethodInfo& info, Args... args) {
		auto r = env->CallStaticBooleanMethod(info.classID(), info.methodID(), args...);
		GEODE_UNWRAP(checkForExceptions(env));
		return geode::Ok(r);
	}

	template <typename T, typename... Args> requires std::same_as<T, int>
	geode::Result<int> performStaticMethodCall(JNIEnv* env, MethodInfo& info, Args... args) {
		auto r = env->CallStaticIntMethod(info.classID(), info.methodID(), args...);
		GEODE_UNWRAP(checkForExceptions(env));
		return geode::Ok(r);
	}

	template <typename T, typename... Args> requires std::same_as<T, float>
	geode::Result<float> performStaticMethodCall(JNIEnv* env, MethodInfo& info, Args... args) {
		auto r = env->CallStaticFloatMethod(info.classID(), info.methodID(), args...);
		GEODE_UNWRAP(checkForExceptions(env));
		return geode::Ok(r);
	}

	template <typename T, typename... Args> requires std::same_as<T, std::vector<int>>
	geode::Result<std::vector<int>> performStaticMethodCall(JNIEnv* env, MethodInfo& info, Args... args) {
			auto array = LocalRef(env->CallStaticObjectMethod(info.classID(), info.methodID(), args...));
			GEODE_UNWRAP(checkForExceptions(env));

			return extractArray(env, array.get<jintArray>());
	}

	template <typename T, typename... Args> requires std::same_as<T, jobject>
	geode::Result<LocalRef> performStaticMethodCall(JNIEnv* env, MethodInfo& info, Args... args) {
		auto r = LocalRef(env->CallStaticObjectMethod(info.classID(), info.methodID(), args...));
		GEODE_UNWRAP(checkForExceptions(env));

		return geode::Ok(std::move(r));
	}

	/**
	 * Calls a static JNI method with the given signature and arguments.
	 * This version accepts an env pointer, which may be faster if you're already using it for other purposes.
	 */
	template <typename T, typename... Args>
	std::invoke_result_t<decltype(performStaticMethodCall<T>), JNIEnv*, MethodInfo&> callStaticMethod(JNIEnv* env, const char* className, const char* methodName, const char* parameterSignature, Args... args) {
		GEODE_UNWRAP_INTO(auto& info, getStaticMethodInfo(env, className, methodName, parameterSignature));

		return performStaticMethodCall<T>(env, info, args...);
	}

	/**
	 * Calls a static JNI method with the given signature and arguments.
	 */
	template <typename T, typename... Args>
	geode::Result<T> callStaticMethod(const char* className, const char* methodName, const char* parameterSignature, Args... args) {
		GEODE_UNWRAP_INTO(auto env, getEnv());
		return callStaticMethod<T>(env, className, methodName, parameterSignature, args...);
	}

	template <typename T, typename... Args> requires std::same_as<T, std::string>
	geode::Result<std::string> performMethodCall(JNIEnv* env, MethodInfo& info, jobject obj, Args... args) {
		auto s = LocalRef(env->CallObjectMethod(obj, info.methodID(), args...));
		return convertString(env, s.get<jstring>());
	}

	template <typename T, typename... Args> requires std::same_as<T, int>
	geode::Result<int> performMethodCall(JNIEnv* env, MethodInfo& info, jobject obj, Args... args) {
		return geode::Ok(env->CallIntMethod(obj, info.methodID(), args...));
	}

	/**
	 * Calls a static JNI method with the given signature and arguments.
	 * This version accepts an env pointer, which may be faster if you're already using it for other purposes.
	 */
	template <typename T, typename... Args>
	geode::Result<T> callMethod(JNIEnv* env, const char* className, const char* methodName, const char* parameterSignature, jobject obj, Args... args) {
		GEODE_UNWRAP_INTO(auto& info, getMethodInfo(env, className, methodName, parameterSignature));
		return performMethodCall<T>(env, info, obj, args...);
	}

	/**
	 * Calls a static JNI method with the given signature and arguments.
	 */
	template <typename T, typename... Args>
	geode::Result<T> callMethod(const char* className, const char* methodName, const char* parameterSignature, jobject obj, Args... args) {
		GEODE_UNWRAP_INTO(auto env, getEnv());
		return callMethod<T>(env, className, methodName, parameterSignature, obj, args...);
	}
};
