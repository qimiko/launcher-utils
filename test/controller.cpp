#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/AndroidEvent.hpp>

#include <launcher-utils/geode.hpp>

#include <chrono>
#include <random>

#include "base.hpp"

using namespace geode::prelude;

constexpr const char* source_name(launcher_utils::InputDevice::Source source) {
	using namespace launcher_utils;

	switch (source) {
		case InputDevice::Source::Keyboard: return "Keyboard";
		case InputDevice::Source::DPad: return "DPad";
		case InputDevice::Source::Gamepad: return "Gamepad";
		case InputDevice::Source::Touchscreen: return "Touchscreen";
		case InputDevice::Source::Mouse: return "Mouse";
		case InputDevice::Source::Stylus: return "Stylus";
		case InputDevice::Source::Trackball: return "Trackball";
		case InputDevice::Source::MouseRelative: return "MouseRelative";
		case InputDevice::Source::Touchpad: return "Touchpad";
		case InputDevice::Source::TouchNavigation: return "TouchNavigation";
		case InputDevice::Source::RotaryEncoder: return "RotaryEncoder";
		case InputDevice::Source::Joystick: return "Joystick";
		case InputDevice::Source::Sensor: return "Sensor";

		default:
		case InputDevice::Source::Unknown: return "Unknown";
	}
}

constexpr bool source_is_controller(launcher_utils::InputDevice::Source source) {
	using namespace launcher_utils;
	return (source & InputDevice::Source::Joystick) == InputDevice::Source::Joystick
		|| (source & InputDevice::Source::Gamepad) == InputDevice::Source::Gamepad;
}

std::vector<launcher_utils::InputDevice::Source> split_sources(launcher_utils::InputDevice::Source sources) {
	using namespace launcher_utils;
	std::array allSources = {
		InputDevice::Source::Keyboard,
		InputDevice::Source::DPad,
		InputDevice::Source::Gamepad,
		InputDevice::Source::Touchscreen,
		InputDevice::Source::Mouse,
		InputDevice::Source::Stylus,
		InputDevice::Source::Trackball,
		InputDevice::Source::MouseRelative,
		InputDevice::Source::Touchpad,
		InputDevice::Source::TouchNavigation,
		InputDevice::Source::RotaryEncoder,
		InputDevice::Source::Joystick,
		InputDevice::Source::Sensor
	};

	std::vector<InputDevice::Source> list{};

	for (const auto source : allSources) {
		if ((sources & source) == source) {
			list.push_back(source);
		}
	}

	return list;
}

enum class AndroidLightType {
	Microphone = 8,
	Input = 10001,
	PlayerId = 10002,
	KeyboardBacklight = 10003
};

constexpr const char* light_type_name(AndroidLightType x) {
	switch (x) {
		case AndroidLightType::Microphone: return "Microphone";
		case AndroidLightType::Input: return "Input";
		case AndroidLightType::PlayerId: return "PlayerId";
		case AndroidLightType::KeyboardBacklight: return "KeyboardBacklight";
		default: return "Unknown";
	}
}

constexpr const char* geode_light_type_name(launcher_utils::InputDevice::ControllerLightType x) {
	using namespace launcher_utils;
	switch (x) {
		case InputDevice::ControllerLightType::PlayerNumber: return "PlayerNumber";
		case InputDevice::ControllerLightType::Color: return "Color";
		case InputDevice::ControllerLightType::All: return "All";
		default:
		case InputDevice::ControllerLightType::None: return "None";
	}
}

namespace {
	std::random_device dev{};
	std::mt19937 rng{dev()};
}

class TriggerPositionIndicator : public cocos2d::CCNode {
	float m_triggerPos{0.0f};
	cocos2d::CCDrawNode* m_indicator{};

	void updateTriggerPosition() {
		m_indicator->clear();

		m_indicator->drawRect(
			{0.0f, 0.0f}, {10.0f, 40.0f * m_triggerPos},
			{1.0f, 0.0f, 0.0f, 1.0f}, 1.0f, {0.0f, 0.0f, 0.0f, 0.0f}
		);
	}

	bool init() {
		if (!cocos2d::CCNode::init()) {
			return false;
		}

		auto indicator = cocos2d::CCDrawNode::create();
		this->addChild(indicator);

		m_indicator = indicator;

		this->updateTriggerPosition();

		return true;
	}

public:
	static TriggerPositionIndicator* create() {
		auto pRet = new TriggerPositionIndicator();
		if (!pRet->init()) {
			delete pRet;
			return nullptr;
		}

		pRet->autorelease();
		return pRet;
	}

	void setTriggerPosition(float pos) {
		m_triggerPos = pos;
		this->updateTriggerPosition();
	}
};

class JoystickPositionIndicator : public cocos2d::CCNode {
	cocos2d::CCPoint m_joystickPos{0.0f, 0.0f};
	cocos2d::CCDrawNode* m_joystick{};

	void updateJoystickPosition() {
		m_joystick->clear();

		m_joystick->drawCircle(
			{0.0f, 0.0f}, 30.0f,
			{0.0f, 0.0f, 0.0f, 0.0f}, 1.0f,
			{1.0f, 1.0f, 1.0f, 1.0f}, 16
		);
		m_joystick->drawDot(m_joystickPos * 30.0f, 5.0f, {1.0f, 0.0f, 0.0f, 1.0f});
	}

	bool init() {
		if (!cocos2d::CCNode::init()) {
			return false;
		}

		auto joystick = cocos2d::CCDrawNode::create();
		this->addChild(joystick);

		m_joystick = joystick;

		this->updateJoystickPosition();

		return true;
	}

public:
	static JoystickPositionIndicator* create() {
		auto pRet = new JoystickPositionIndicator();
		if (!pRet->init()) {
			delete pRet;
			return nullptr;
		}

		pRet->autorelease();
		return pRet;
	}

	void setJoystickPosition(cocos2d::CCPoint pos) {
		m_joystickPos = pos;
		this->updateJoystickPosition();
	}
};

class ControllerTestLayer : public BaseTestLayer {
	bool m_nextInputController{false};

	int m_currentDeviceId{-1};
	std::unique_ptr<launcher_utils::InputDevice> m_currentInputDevice{};

	geode::MDTextArea* m_deviceInfoLabel{};
	geode::MDTextArea* m_vibrationLabel{};
	geode::MDTextArea* m_lightsLabel{};

	int m_page{0};

	cocos2d::CCNode* m_joystickLayer{};
	JoystickPositionIndicator* m_joystickLeft{};
	JoystickPositionIndicator* m_joystickRight{};
	JoystickPositionIndicator* m_joystickHat{};

	TriggerPositionIndicator* m_triggerLeft{};
	TriggerPositionIndicator* m_triggerRight{};

	void togglePage(int page) {
		page = std::clamp(page, 0, 3);
		m_page = page;

		m_deviceInfoLabel->setVisible(page == 0);
		m_vibrationLabel->setVisible(page == 1);
		m_lightsLabel->setVisible(page == 2);
		m_joystickLayer->setVisible(page == 3);
	}

	virtual bool init() override {
		if (!BaseTestLayer::init()) {
			return false;
		}

		auto safeArea = geode::utils::getSafeAreaRect();

		this->setKeyboardEnabled(true);

		auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

		auto deviceInfo = geode::MDTextArea::create("# touch the device!!", {350.0f, 150.0f});
		deviceInfo->setPosition(winSize / 2);

		this->addChild(deviceInfo);
		m_deviceInfoLabel = deviceInfo;

		auto vibrationLabel = geode::MDTextArea::create(R"*(# vibration
![X](frame:controllerBtn_X_001.png) vibrate all  
![Y](frame:controllerBtn_Y_001.png) vibrate random  
![B](frame:controllerBtn_B_001.png) cancel)*", {200.0f, 200.0f});
		vibrationLabel->setPosition(winSize / 2);

		m_vibrationLabel = vibrationLabel;
		this->addChild(vibrationLabel);

		auto lightsLabel = geode::MDTextArea::create(R"*(# lights
![X](frame:controllerBtn_X_001.png) random color  
![Y](frame:controllerBtn_Y_001.png) random player number  
![B](frame:controllerBtn_B_001.png) disable)*", {200.0f, 200.0f});
		lightsLabel->setPosition(winSize / 2);

		m_lightsLabel = lightsLabel;
		this->addChild(lightsLabel);

		auto pageHelp = cocos2d::CCLabelBMFont::create("Use RB/LB to toggle pages", "bigFont.fnt");
		this->addChild(pageHelp);

		pageHelp->setPosition(
			safeArea.origin.x + safeArea.size.width - 10.0f,
			safeArea.origin.y + 10.0f
		);
		pageHelp->setScale(0.5f);
		pageHelp->setAnchorPoint({1.0f, 0.5f});

		auto joystickLayer = cocos2d::CCNode::create();
		this->addChild(joystickLayer);
		m_joystickLayer = joystickLayer;

		auto joystickLeft = JoystickPositionIndicator::create();
		joystickLayer->addChild(joystickLeft);
		m_joystickLeft = joystickLeft;

		joystickLeft->setPosition(winSize.width/2 - 40.0f, winSize.height/2 - 40.0f);

		auto joystickRight = JoystickPositionIndicator::create();
		joystickLayer->addChild(joystickRight);
		m_joystickRight = joystickRight;

		joystickRight->setPosition(winSize.width/2 + 40.0f, winSize.height/2 - 40.0f);

		auto joystickHat = JoystickPositionIndicator::create();
		joystickLayer->addChild(joystickHat);
		m_joystickHat = joystickHat;

		joystickHat->setPosition(winSize.width/2 - 120.0f, winSize.height/2);

		auto triggerLeft = TriggerPositionIndicator::create();
		joystickLayer->addChild(triggerLeft);
		m_triggerLeft = triggerLeft;

		triggerLeft->setPosition(winSize.width/2 - 80.0f, winSize.height/2 + 60.0f);

		auto triggerRight = TriggerPositionIndicator::create();
		joystickLayer->addChild(triggerRight);
		m_triggerRight = triggerRight;

		triggerRight->setPosition(winSize.width/2 + 80.0f, winSize.height/2 + 60.0f);

		this->togglePage(0);

		auto controllerCount = launcher_utils::getConnectedControllerCount();
		if (!controllerCount) {
			geode::log::warn("failed to get controller count: {}", controllerCount.unwrapErr());
		} else {
			addLogLine(fmt::format("controllerCount: {}", controllerCount.unwrap()));
		}

		auto devices = launcher_utils::getConnectedDevices();
		if (!devices) {
			geode::log::warn("failed to get devices: {}", devices.unwrapErr());
		} else {
			addLogLine(fmt::format("devices: {}", devices.unwrap()));
		}

		return true;
	}

	void updateInputDevice(int device, bool force = false) {
		if (m_currentInputDevice && m_currentInputDevice->getDeviceId() == device && !force) {
			return;
		}

		m_currentInputDevice.reset();
		m_currentDeviceId = -1;

		if (device == -1) {
			m_deviceInfoLabel->setString("# No device selected");
			addLogLine("clearing device");
			return;
		}

		auto res = launcher_utils::InputDevice::create(device);
		if (!res) {
			m_deviceInfoLabel->setString("# Failed to load device");
			addLogLine(fmt::format("Failed to load device: {}", res.unwrapErr()));

			return;
		}

		auto&& inputDevice = res.unwrap();

		auto descriptor = inputDevice.getDescriptor();
		auto name = inputDevice.getName();

		auto productId = inputDevice.getProductId();
		auto vendorId = inputDevice.getVendorId();

		auto hasBattery = (productId == 24833 && vendorId == 11720)
			? false
			: inputDevice.hasBattery();

		auto batteryString = hasBattery
			? fmt::format(
					"battery={}-{}%",
					static_cast<int>(inputDevice.getBatteryStatus()),
					inputDevice.getBatteryCapacity() * 100.0f
				)
			: "battery=none";

		auto sources = split_sources(inputDevice.getSources());

		std::vector<std::string> sourceStr{};
		sourceStr.reserve(sources.size());

		for (const auto x : sources) {
			sourceStr.push_back(source_name(x));
		}

		auto lightCount = inputDevice.getLightCount();
		auto lightType = inputDevice.getLightType();

		auto motorCount = inputDevice.getMotorCount();

		auto deviceId = inputDevice.getDeviceId();

		auto msg = fmt::format(R"*(# Device Info (#{})
name={}  
descriptor={}  
product={:#x}, vendor={:#x}  
{}  
sources={}  
lights={} ({}) motors={})*",
			deviceId,
			name,
			descriptor,
			productId, vendorId,
			batteryString,
			sourceStr,
			lightCount, geode_light_type_name(lightType), motorCount
		);
		m_deviceInfoLabel->setString(msg.c_str());

		m_currentInputDevice = std::make_unique<launcher_utils::InputDevice>(std::move(inputDevice));
		m_currentDeviceId = deviceId;
	}

	void onVibrateBtn(enumKeyCodes key) {
		if (!m_currentInputDevice) {
			return;
		}

		auto motorCount = m_currentInputDevice->getMotorCount();
		if (motorCount == 0) {
			addLogLine("no motors to vibrate!");
			return;
		}

		if (key == enumKeyCodes::CONTROLLER_Y) {
			// single
			std::uniform_int_distribution<> amplitudeDist{0, 255};
			std::uniform_int_distribution<> deviceDist{0, motorCount-1};

			auto amplitude = amplitudeDist(rng);
			auto deviceIdx = deviceDist(rng);

			addLogLine(fmt::format("vibrating {} for 500ms with amplitude {}", deviceIdx, amplitude));
			auto r = m_currentInputDevice->vibrateDevice(500, amplitude, deviceIdx);
			if (!r) {
				addLogLine(fmt::format("vibrating failed: {}", r.unwrapErr()));
			}
		}

		if (key == enumKeyCodes::CONTROLLER_X) {
			// all
			std::uniform_int_distribution<> amplitudeDist{0, 255};
			auto amplitude = amplitudeDist(rng);

			addLogLine(fmt::format("vibrating all for 500ms with amplitude {}", amplitude));
			auto r = m_currentInputDevice->vibrateDevice(500, amplitude);
			if (!r) {
				addLogLine(fmt::format("vibrating failed: {}", r.unwrapErr()));
			}
		}

		if (key == enumKeyCodes::CONTROLLER_B) {
			// disable
			addLogLine("disabling vibration");
			auto r = m_currentInputDevice->vibrateDevice(0, 0);
			if (!r) {
				addLogLine(fmt::format("vibrating failed: {}", r.unwrapErr()));
			}
		}
	}

	void onLightsBtn(enumKeyCodes key) {
		if (!m_currentInputDevice) {
			return;
		}

		auto lightsCount = m_currentInputDevice->getLightCount();
		if (lightsCount == 0) {
			addLogLine("no lights to set!");
			return;
		}

		if (key == enumKeyCodes::CONTROLLER_Y) {
			// random player
			std::uniform_int_distribution<> playerDist{1, 8};
			auto player = playerDist(rng);

			addLogLine(fmt::format("setting lights to player {}", player));
			auto r = m_currentInputDevice->setLights(
				launcher_utils::InputDevice::ControllerLightType::PlayerNumber,
				player
			);
			if (!r) {
				addLogLine(fmt::format("setting light player failed: {}", r.unwrapErr()));
			}
		}

		if (key == enumKeyCodes::CONTROLLER_X) {
			// random color
			std::uniform_int_distribution<std::uint32_t> colorDist{0, 0xff'ff'ff'ff}; // AARRGGB
			auto color = colorDist(rng);

			addLogLine(fmt::format("setting lights to color {}", color));
			auto r = m_currentInputDevice->setLights(
				launcher_utils::InputDevice::ControllerLightType::Color, color
			);
			if (!r) {
				addLogLine(fmt::format("setting light color failed: {}", r.unwrapErr()));
			}
		}

		if (key == enumKeyCodes::CONTROLLER_B) {
			// disable
			addLogLine("disabling lights");
			auto r = m_currentInputDevice->setLights(
				launcher_utils::InputDevice::ControllerLightType::All, 0
			);
			if (!r) {
				addLogLine(fmt::format("clearing lights failed: {}", r.unwrapErr()));
			}
		}
	}

	virtual void keyDown(enumKeyCodes key) override {
		if (!m_nextInputController) {
			return;
		}

		if (m_page == 0) {
			auto keyName = cocos2d::CCDirector::sharedDirector()
				->getKeyboardDispatcher()
				->keyToString(key);
			if (keyName == nullptr) {
				keyName = "Unknown";
			}

			addLogLine(fmt::format("Recv keyDown from device {}: {}", m_currentDeviceId, keyName));
		}

		if (key == enumKeyCodes::CONTROLLER_RB) {
			togglePage(m_page + 1);
			return;
		}

		if (key == enumKeyCodes::CONTROLLER_LB) {
			togglePage(m_page - 1);
			return;
		}

		if (m_page == 1) {
			onVibrateBtn(key);
			return;
		}

		if (m_page == 2) {
			onLightsBtn(key);
			return;
		}
	}

	virtual void keyUp(enumKeyCodes key) override {
		if (!m_nextInputController) {
			return;
		}

		if (m_page == 0) {
			auto keyName = cocos2d::CCDirector::sharedDirector()
				->getKeyboardDispatcher()
				->keyToString(key);
			if (keyName == nullptr) {
				keyName = "Unknown";
			}

			addLogLine(fmt::format("Recv keyUp from device {}: {}", m_currentDeviceId, keyName));
		}
	}

	virtual void keyMenuClicked() override {
		addLogLine("menu clicked");
	}

	void preKeyInput(AndroidInputDeviceInfoEvent* event) {
		if (m_page == 0) {
			auto msg = fmt::format(
				"pre input from device={} src={}",
				event->deviceId(),
				source_name(static_cast<launcher_utils::InputDevice::Source>(event->eventSource()))
			);
			addLogLine(msg);
		}

		updateInputDevice(event->deviceId());

		if (source_is_controller(
			static_cast<launcher_utils::InputDevice::Source>(event->eventSource())
		)) {
			m_nextInputController = true;
		} else {
			m_nextInputController = false;
		}
	}

	EventListener<AndroidInputDeviceInfoFilter> m_listener{
		this, &ControllerTestLayer::preKeyInput,
		geode::AndroidInputDeviceInfoFilter()
	};

	void devicesChanged(AndroidInputDeviceEvent* event) {
		auto deviceId = event->deviceId();
		auto status = event->status();

		geode::Loader::get()->queueInMainThread([this, deviceId, status]() {
			auto msg = fmt::format("Update controller {}: {}", deviceId, static_cast<int>(status));
			addLogLine(msg);

			if (status == AndroidInputDeviceEvent::Status::Removed && m_currentDeviceId == deviceId) {
				this->updateInputDevice(-1);
			} else {
				this->updateInputDevice(deviceId, true);
			}
		});
	}

	EventListener<geode::AndroidInputDeviceFilter> m_inputChangeListener{
		this, &ControllerTestLayer::devicesChanged,
		geode::AndroidInputDeviceFilter()
	};

	void joysticksUpdate(AndroidInputJoystickEvent* event) {
		auto leftJoystickX = event->leftX();
		auto leftJoystickY = event->leftY();

		if (!leftJoystickX.empty() && !leftJoystickY.empty()) {
			cocos2d::CCPoint pos{leftJoystickX.back(), -leftJoystickY.back()};
			m_joystickLeft->setJoystickPosition(pos);
		}

		auto rightJoystickX = event->rightX();
		auto rightJoystickY = event->rightY();

		if (!rightJoystickX.empty() && !rightJoystickY.empty()) {
			cocos2d::CCPoint pos{rightJoystickX.back(), -rightJoystickY.back()};
			m_joystickRight->setJoystickPosition(pos);
		}

		auto hatJoystickX = event->hatX();
		auto hatJoystickY = event->hatY();

		if (!hatJoystickX.empty() && !hatJoystickY.empty()) {
			cocos2d::CCPoint pos{hatJoystickX.back(), -hatJoystickY.back()};
			m_joystickHat->setJoystickPosition(pos);
		}

		auto triggerLeft = event->leftTrigger();
		if (!triggerLeft.empty()) {
			m_triggerLeft->setTriggerPosition(triggerLeft.back());
		}

		auto triggerRight = event->rightTrigger();
		if (!triggerRight.empty()) {
			m_triggerRight->setTriggerPosition(triggerRight.back());
		}
	}

	EventListener<geode::AndroidInputJoystickFilter> m_joystickUpdateListener{
		this, &ControllerTestLayer::joysticksUpdate,
		geode::AndroidInputJoystickFilter()
	};

public:
	static ControllerTestLayer* create() {
		auto pRet = new ControllerTestLayer();
		if (!pRet->init()) {
			delete pRet;
			return nullptr;
		}

		pRet->autorelease();
		return pRet;
	}
};

struct CustomMenuLayer : geode::Modify<CustomMenuLayer, MenuLayer> {
	virtual bool init() override {
		if (!MenuLayer::init()) {
			return false;
		}

		auto controllerButton = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_gpgBtn_001.png"),
			this,
			menu_selector(CustomMenuLayer::onController)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(controllerButton);

		controllerButton->setID("controller-btn"_spr);
		menu->updateLayout();

		return true;
	}

	void onController(CCObject*) {
		auto scene = cocos2d::CCScene::create();
		scene->addChild(ControllerTestLayer::create());

		cocos2d::CCDirector::sharedDirector()->pushScene(
			cocos2d::CCTransitionFade::create(0.5f, scene)
		);
	}
};
