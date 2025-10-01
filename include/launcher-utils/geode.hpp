#pragma once

#include <Geode/Result.hpp>

#include <cstdint>
#include <vector>
#include <span>

#include "jni.hpp"

namespace launcher_utils {
	class InputDevice final {
	private:
		int m_deviceId;
		jni::GlobalRef m_inputDevice{};

		InputDevice(int deviceId, jni::GlobalRef&& inputDevice) : m_deviceId(deviceId), m_inputDevice(std::move(inputDevice)) {}

	public:
		static geode::Result<InputDevice> create(int deviceId) {
			GEODE_UNWRAP_INTO(auto env, jni::getEnv());
			GEODE_UNWRAP_INTO(auto obj, jni::callStaticMethod<jobject>(env, "com/geode/launcher/utils/GeodeUtils", "getDevice", "(I)Landroid/view/InputDevice;", deviceId));

			auto ref = jni::GlobalRef(*obj);

			return geode::Ok(std::move(InputDevice(deviceId, std::move(ref))));
		}

		std::string getDescriptor() {
			return jni::callMethod<std::string>("android/view/InputDevice", "getDescriptor", "()Ljava/lang/String;", *m_inputDevice).unwrapOrDefault();
		}

		std::string getName() {
			return jni::callMethod<std::string>("android/view/InputDevice", "getName", "()Ljava/lang/String;", *m_inputDevice).unwrapOrDefault();
		}

		int getVendorId() {
			return jni::callMethod<int>("android/view/InputDevice", "getVendorId", "()I", *m_inputDevice).unwrapOrDefault();
		}

		int getProductId() {
			return jni::callMethod<int>("android/view/InputDevice", "getProductId", "()I", *m_inputDevice).unwrapOrDefault();
		}

		float getBatteryCapacity() {
			return jni::callStaticMethod<float>("com/geode/launcher/utils/GeodeUtils", "getDeviceBatteryCapacity", "(I)F", m_deviceId).unwrapOrDefault();
		}

		enum class BatteryStatus {
			Unknown = 1,
			Charging = 2,
			Discharging = 3,
			NotCharging = 4,
			Full = 5
		};

		BatteryStatus getBatteryStatus() {
			return static_cast<BatteryStatus>(
				jni::callStaticMethod<int>("com/geode/launcher/utils/GeodeUtils", "getDeviceBatteryStatus", "(I)I", m_deviceId).unwrapOr(1)
			);
		}

		bool hasBattery() {
			return jni::callStaticMethod<bool>("com/geode/launcher/utils/GeodeUtils", "deviceHasBattery", "(I)Z", m_deviceId).unwrapOrDefault();
		}

		enum class Source {
			Unknown = 0,
			Keyboard = 0x00000101,
			DPad = 0x00000201,
			Gamepad = 0x00000401,
			Touchscreen = 0x00001002,
			Mouse = 0x00002002,
			Stylus = 0x00004002,
			Trackball = 0x00010004,
			MouseRelative = 0x00020004,
			Touchpad = 0x00100008,
			TouchNavigation = 0x00200000,
			RotaryEncoder = 0x00400000,
			Joystick = 0x01000010,
			Sensor = 0x04000000
		};

		Source getSources() {
			return static_cast<Source>(
				jni::callMethod<int>("android/view/InputDevice", "getSources", "()I", *m_inputDevice).unwrapOrDefault()
			);
		}

		enum class ControllerLightType {
			None = 0,
			PlayerNumber = 1,
			Color = 2,
			All = 3
		};

		int getLightCount() {
			return jni::callStaticMethod<int>("com/geode/launcher/utils/GeodeUtils", "getDeviceLightsCount", "(I)I", m_deviceId).unwrapOrDefault();
		}

		ControllerLightType getLightType() {
			return static_cast<ControllerLightType>(
				jni::callStaticMethod<int>("com/geode/launcher/utils/GeodeUtils", "getLightType", "(I)I", m_deviceId).unwrapOrDefault()
			);
		}

		geode::Result<> setLights(ControllerLightType type, std::uint32_t color) {
			GEODE_UNWRAP_INTO(auto r, jni::callStaticMethod<bool>("com/geode/launcher/utils/GeodeUtils", "setDeviceLightColor", "(III)Z", m_deviceId, static_cast<jint>(color), static_cast<jint>(type)));
			if (!r) {
				return geode::Err("call failed");
			}

			return geode::Ok();
		}

		int getMotorCount() {
			return jni::callStaticMethod<int>("com/geode/launcher/utils/GeodeUtils", "getDeviceHapticsCount", "(I)I", m_deviceId).unwrapOrDefault();
		}

		geode::Result<> vibrateDevice(long durationMs, int intensity, int motorIdx = -1) {
			GEODE_UNWRAP_INTO(auto r, jni::callStaticMethod<bool>("com/geode/launcher/utils/GeodeUtils", "vibrateDevice", "(IJII)Z", m_deviceId, durationMs, intensity, motorIdx));
			if (!r) {
				return geode::Err("call failed");
			}

			return geode::Ok();
		}

		int getDeviceId() const {
			return m_deviceId;
		}
	};

	constexpr InputDevice::Source operator&(const InputDevice::Source a, const InputDevice::Source b) {
		return static_cast<InputDevice::Source>(static_cast<std::underlying_type_t<InputDevice::Source>>(a) & static_cast<std::underlying_type_t<InputDevice::Source>>(b));
	}

	constexpr InputDevice::Source operator|(const InputDevice::Source a, const InputDevice::Source b) {
		return static_cast<InputDevice::Source>(static_cast<std::underlying_type_t<InputDevice::Source>>(a) | static_cast<std::underlying_type_t<InputDevice::Source>>(b));
	}

	geode::Result<int> getConnectedControllerCount();
	geode::Result<std::vector<int>> getConnectedDevices();

	geode::Result<bool> vibrateSupported();
	geode::Result<> vibrate(long ms);
	geode::Result<> vibratePattern(std::span<long> pattern, int repeat);
};

