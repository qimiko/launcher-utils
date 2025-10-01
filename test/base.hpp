#pragma once

#include <Geode/Geode.hpp>

#include <string_view>

class BaseTestLayer : public cocos2d::CCLayer {
	cocos2d::CCLabelBMFont* m_logs{};
	std::vector<std::string> m_logLines{};

	void onBack(cocos2d::CCObject*) {
		cocos2d::CCDirector::sharedDirector()->popSceneWithTransition(
			0.5f, cocos2d::PopTransition::kPopTransitionFade
		);
	}

protected:
	virtual bool init() override {
		if (!cocos2d::CCLayer::init()) {
			return false;
		}

		auto safeArea = geode::utils::getSafeAreaRect();

		this->setKeypadEnabled(true);

		this->addChild(geode::createLayerBG());

		auto logs = cocos2d::CCLabelBMFont::create("meow meow\n meow", "geode.loader/mdFontMono.fnt");
		this->addChild(logs);

		logs->setAlignment(cocos2d::CCTextAlignment::kCCTextAlignmentLeft);
		logs->setAnchorPoint({0.0f, 0.0f});
		logs->setPosition(safeArea.origin.x, safeArea.origin.y + 10.0f);

		logs->setScale(0.50f);

		logs->setOpacity(127);
		m_logs = logs;

		auto backMenu = cocos2d::CCMenu::create();
		backMenu->setContentSize({100.0f, 40.0f});
		backMenu->setAnchorPoint({0.0f, 0.5f});

		auto backSpr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
		auto backBtn = CCMenuItemSpriteExtra::create(
			backSpr, this, menu_selector(BaseTestLayer::onBack)
		);
		backMenu->addChild(backBtn);

		backMenu->setLayout(
			geode::SimpleRowLayout::create()
				->setMainAxisAlignment(geode::MainAxisAlignment::Start)
				->setGap(5.0f)
		);
		this->addChildAtPosition(backMenu, geode::Anchor::TopLeft, ccp(12, -25), false);

		return true;
	}

	virtual void keyBackClicked() override {
		onBack(nullptr);
	}

	void addLogLine(std::string_view line) {
		geode::log::debug("{}", line);

		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();
		auto str = fmt::format("{}: {}", time, line);

		m_logLines.push_back(str);

		if (m_logLines.size() > 30) {
			m_logLines.erase(m_logLines.begin());
		}

		auto msg = geode::utils::string::join(m_logLines, "\n");
		m_logs->setString(msg.c_str(), false);
	}
};