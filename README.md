# Launcher JNI Utils

A collection of JNI helpers that makes interfacing with Geode's [Android Launcher](https://github.com/geode-sdk/android-launcher) easier.

Compared Cocos's JNI helper, this helper has the following additions:

- Improved performance through caching
- Improved safety through exception checks and usage of Geode results
- Improved memory management through `jobject` wrappers
- Significantly less boilerplate when calling methods

## Usage

After including Geode's subdirectory, CPM can be used to add this library as well:

```cmake
CPMAddPackage("gh:qimiko/launcher-utils#commit")
target_link_libraries(${PROJECT_NAME} launcher-utils)
```

To use the JNI helpers directly, include [`<launcher-utils/jni.hpp>`](/include/launcher-utils/jni.hpp).

You can call a JNI method through the `call{Static}Method` function:

```cpp
launcher_utils::jni::callStaticMethod<long>("com/geode/launcher/utils/GeodeUtils", "exampleIntegerMethod", "(I)J", 32);

launcher_utils::jni::callMethod<std::string>("com/geode/launcher/utils/GeodeUtils", "exampleNonStaticMethod", "(I)Ljava/lang/String;", exampleObject, 43);
```

See the [JNI docs](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/types.html) for more information on building a method signature.

Launcher method wrappers are available in the [`<launcher-utils/geode.hpp>`](/include/launcher-utils/geode.hpp) header. See the [test mod](/test) for example usages of these methods.

