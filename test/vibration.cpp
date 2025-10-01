#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <launcher-utils/geode.hpp>

#include "base.hpp"

#include <random>

namespace {
	std::random_device dev{};
	std::mt19937 rng{dev()};
}

class VibrationTestLayer : public BaseTestLayer {
	virtual bool init() override {
		if (!BaseTestLayer::init()) {
			return false;
		}

		auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

		if (launcher_utils::vibrateSupported()) {
			auto vibrateMenu = cocos2d::CCMenu::create();

			auto vibrateButton = geode::cocos::CCMenuItemExt::createSpriteExtra(
				ButtonSprite::create("Vibrate"),
				[this](auto) {
					static std::uniform_int_distribution<> lengthDist{0, 1000};

					auto len = lengthDist(rng);
					addLogLine(fmt::format("vibrating device for {}ms", len));
					if (auto res = launcher_utils::vibrate(len); !res) {
						addLogLine(fmt::format("vibrate failed: {}", res.unwrapErr()));
					}
				}
			);

			vibrateMenu->addChild(vibrateButton);

			auto patternButton = geode::cocos::CCMenuItemExt::createSpriteExtra(
				ButtonSprite::create("Pattern"),
				[this](auto) {
					static std::uniform_int_distribution<> lengthDist{0, 100};
					static std::uniform_int_distribution<> amountDist{2, 10};
					static std::uniform_int_distribution<> repeatDist{-1, 5};

					auto cnt = amountDist(rng);
					std::vector<long> pattern(cnt);
					std::generate(pattern.begin(), pattern.end(), []() {
						return lengthDist(rng);
					});

					auto repeat = repeatDist(rng);

					addLogLine(fmt::format("vibrating device with pattern {} (repeat idx {})", pattern, repeat));
					if (auto res = launcher_utils::vibratePattern(pattern, repeat); !res) {
						addLogLine(fmt::format("vibrate pattern failed: {}", res.unwrapErr()));
					}
				}
			);
			vibrateMenu->addChild(patternButton);

			auto cancelButton = geode::cocos::CCMenuItemExt::createSpriteExtra(
				ButtonSprite::create("Cancel"),
				[this](auto) {
					addLogLine("cancelling vibration");
					if (auto res = launcher_utils::vibrate(0); !res) {
						addLogLine(fmt::format("vibrate failed: {}", res.unwrapErr()));
					}
				}
			);
			vibrateMenu->addChild(cancelButton);

			vibrateMenu->setLayout(
				geode::SimpleColumnLayout::create()
					->setGap(5.0f)
			);
			this->addChild(vibrateMenu);
			vibrateMenu->setPosition(winSize/2);

		} else {
			auto noVibrateLabel = cocos2d::CCLabelBMFont::create("Vibration is not supported!", "bigFont.fnt");
			this->addChild(noVibrateLabel);

			noVibrateLabel->setPosition(winSize/2);
		}

		return true;
	}

public:
	static VibrationTestLayer* create() {
		auto pRet = new VibrationTestLayer();
		if (!pRet->init()) {
			delete pRet;
			return nullptr;
		}

		pRet->autorelease();
		return pRet;
	}
};

struct VibrationMenuLayer : geode::Modify<VibrationMenuLayer, MenuLayer> {
	virtual bool init() override {
		if (!MenuLayer::init()) {
			return false;
		}

		auto vibrateSprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_fxOffBtn_001.png");
		vibrateSprite->setScale(1.3f);
		auto vibrateButton = CCMenuItemSpriteExtra::create(
			vibrateSprite,
			this,
			menu_selector(VibrationMenuLayer::onVibration)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(vibrateButton);

		vibrateButton->setID("vibrate-btn"_spr);
		menu->updateLayout();

		return true;
	}

	void onVibration(CCObject*) {
		auto scene = cocos2d::CCScene::create();
		scene->addChild(VibrationTestLayer::create());

		cocos2d::CCDirector::sharedDirector()->pushScene(
			cocos2d::CCTransitionFade::create(0.5f, scene)
		);
	}
};
