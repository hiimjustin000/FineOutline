#include <Geode/Geode.hpp>
#include <Geode/modify/SimplePlayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/CharacterColorPage.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include "CCSpriteBatchNode.h"
#include "ShaderCache.h"

using namespace geode::prelude;

$on_mod(Loaded) {
	
	std::string fragIcon = R"(
		#ifdef GL_ES
		precision lowp float;
		#endif

		varying vec4 v_fragmentColor;
		varying vec2 v_texCoord;
		uniform sampler2D CC_Texture0;

		void main() {
			vec4 c = texture2D(CC_Texture0, v_texCoord);
			float br = max(max(c.r, c.g), c.b);
			float gr = float(abs(c.r - c.g) < 0.25 && abs(c.g - c.b) < 0.25);

			c.rgb = mix(c.rgb, vec3(1.0), float(br < 1.0 && c.a > 0.0 && gr > 0.0));

			gl_FragColor = v_fragmentColor * c;
		}
	)";

	ShaderCache::get()->createShader("icon", fragIcon);

	std::string fragOutline = R"(
		#ifdef GL_ES
		precision lowp float;
		#endif

		varying vec4 v_fragmentColor;
		varying vec2 v_texCoord;
		uniform sampler2D CC_Texture0;

		void main() {
			vec4 c = texture2D(CC_Texture0, v_texCoord);
			float br = max(max(c.r, c.g), c.b);
			float gr = float(abs(c.r - c.g) < 0.25 && abs(c.g - c.b) < 0.25);

			float condition = float(br < 1.0 && c.a > 0.0 && gr > 0.0);
			c.a = mix(0.0, c.a * (1.0 - br), condition);
			c.rgb = mix(c.rgb, vec3(1.0), condition);

			gl_FragColor = v_fragmentColor * c;
		}
	)";

	ShaderCache::get()->createShader("outline", fragOutline);
};

void removeShaders(CCSprite* spr) {
	spr->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
	spr->getShaderProgram()->setUniformsForBuiltins();
	spr->getShaderProgram()->use();
	spr->removeChildByID("black_outline"_spr);
}

void updateSprite(CCSprite* spr, ccColor3B color = {0, 0, 0}) {
	if (!spr || color == ccColor3B{0, 0, 0}) return;

	CCSprite* blackOutline = CCSprite::createWithSpriteFrame(spr->displayFrame());

	blackOutline->setContentSize(spr->getContentSize());
	blackOutline->setZOrder(2);
	blackOutline->setID("black_outline"_spr);
	blackOutline->setPosition(spr->getContentSize()/2);
	blackOutline->setColor(color);

	if (CCGLProgram* prgOutline = ShaderCache::get()->getProgram("outline")) {
		blackOutline->setShaderProgram(prgOutline);
		blackOutline->getShaderProgram()->setUniformsForBuiltins();
		blackOutline->getShaderProgram()->use();
		blackOutline->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
	}

	if (CCGLProgram* progIcon = ShaderCache::get()->getProgram("icon")) {
		spr->setShaderProgram(progIcon);
		spr->getShaderProgram()->setUniformsForBuiltins();
		spr->getShaderProgram()->use();
		spr->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
	}

	spr->removeChildByID("black_outline"_spr);
	spr->addChild(blackOutline);
}

class $modify(MySimplePlayer, SimplePlayer) {

	struct Fields {
		bool m_isGlobedSelf = false;
		bool m_isShaderSpr = false;
	};

	void removeAllShaders() {
		m_fields->m_isShaderSpr = false;
		removeShaders(m_firstLayer);

		if (m_robotSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_robotSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						removeShaders(part);
					}
				}
			}
		}
		if (m_spiderSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_spiderSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						removeShaders(part);
					}		
				}
			}
		}
	}

	void setOutlineColor(const ccColor3B& color) {

		if (color == ccColor3B{0, 0, 0}) {
			log::info("here");
			removeAllShaders();
			return;
		}

		if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(m_firstLayer->getChildByID("black_outline"_spr))) {
			blackOutline->setColor(color);
		}
		else {
			updatePlayerShaders();
		}

		if (m_robotSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_robotSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(part->getChildByID("black_outline"_spr))) {
							blackOutline->setColor(color);
						}
					}
				}
			}
		}
		if (m_spiderSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_spiderSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(part->getChildByID("black_outline"_spr))) {
							blackOutline->setColor(color);
						}
					}		
				}
			}
		}
	}

	void updatePlayerShaders() {
		m_fields->m_isShaderSpr = true;
		ccColor3B outlineColor = Mod::get()->getSavedValue<ccColor3B>("p1-color");

		if (m_robotSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_robotSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						updateSprite(part, outlineColor);
					}
				}
			}
		}
		if (m_spiderSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_spiderSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						updateSprite(part, outlineColor);
					}		
				}
			}
		}
		updateSprite(m_firstLayer, outlineColor);
		
	}

    void updatePlayerFrame(int p0, IconType p1) {
		SimplePlayer::updatePlayerFrame(p0, p1);

		if (m_fields->m_isShaderSpr) {
			updatePlayerShaders();
			if (p0 == 1) {
				queueInMainThread([this] {
					updatePlayerShaders();
				});
			}
		}
	}
};

class $modify(MyPlayerObject, PlayerObject) {

	void updatePlayerShaders() {

		if (!m_gameLayer || !(m_gameLayer->m_player1 == this || m_gameLayer->m_player2 == this)) return;

		ccColor3B outlineColor = Mod::get()->getSavedValue<ccColor3B>("p1-color");

		updateSprite(m_iconSprite, outlineColor);
		updateSprite(m_vehicleSprite, outlineColor);
		updateSprite(m_birdVehicle, outlineColor);
		if (m_robotSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_robotSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						updateSprite(part, outlineColor);
					}
				}
			}
		}
		if (m_spiderSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_spiderSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						updateSprite(part, outlineColor);
					}		
				}
			}
		}
	}

	void setOutlineSpriteOpacity(int opacity) {

		if (!m_gameLayer || !(m_gameLayer->m_player1 == this || m_gameLayer->m_player2 == this)) return;

		if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(m_iconSprite->getChildByID("black_outline"_spr))) {
			blackOutline->setOpacity(opacity);
		}
		if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(m_vehicleSprite->getChildByID("black_outline"_spr))) {
			blackOutline->setOpacity(opacity);
		}
		if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(m_birdVehicle->getChildByID("black_outline"_spr))) {
			blackOutline->setOpacity(opacity);
		}

		if (m_robotSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_robotSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(part->getChildByID("black_outline"_spr))) {
							blackOutline->setOpacity(opacity);
						}
					}
				}
			}
		}
		if (m_spiderSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_spiderSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						if (CCSprite* blackOutline = typeinfo_cast<CCSprite*>(part->getChildByID("black_outline"_spr))) {
							blackOutline->setOpacity(opacity);
						}
					}		
				}
			}
		}
	}

    void playerDestroyed(bool p0) {
		PlayerObject::playerDestroyed(p0);
		setOutlineSpriteOpacity(0);
	}

    void playSpawnEffect() {
		PlayerObject::playSpawnEffect();
		setOutlineSpriteOpacity(255);
	}

	bool init(int player, int ship, GJBaseGameLayer* gameLayer, CCLayer* layer, bool highGraphics) {
		if (!PlayerObject::init(player, ship, gameLayer, layer, highGraphics)) return false;
		queueInMainThread([this] {
			updatePlayerShaders();
		});
		return true;
	}

	void updatePlayerFrame(int frame) {
        PlayerObject::updatePlayerFrame(frame);

		updatePlayerShaders();
    }

    void updatePlayerShipFrame(int frame) {
        PlayerObject::updatePlayerShipFrame(frame);
		updatePlayerShaders();
    }

    void updatePlayerRollFrame(int frame) {

        PlayerObject::updatePlayerRollFrame(frame);
		updatePlayerShaders();
    }

    void updatePlayerBirdFrame(int frame) {
        PlayerObject::updatePlayerBirdFrame(frame);
		updatePlayerShaders();
    }

    void updatePlayerDartFrame(int frame) {
        PlayerObject::updatePlayerDartFrame(frame);
		updatePlayerShaders();
    }

    void createRobot(int frame) {
        PlayerObject::createRobot(frame);

		static_cast<MyCCSpriteBatchNode*>(m_robotBatchNode)->setFake(true);

		if (m_robotSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_robotSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						updateSprite(part);
					}
				}
			}
		}
    }

    void createSpider(int frame) {
        PlayerObject::createSpider(frame);

		static_cast<MyCCSpriteBatchNode*>(m_spiderBatchNode)->setFake(true);

		if (m_spiderSprite) {
			if (CCPartAnimSprite* animSpr = getChildOfType<CCPartAnimSprite>(m_spiderSprite, 0)) {
				for(CCNode* node : CCArrayExt<CCNode*>(animSpr->getChildren())) {
					if(CCSpritePart* part = typeinfo_cast<CCSpritePart*>(node)) {
						updateSprite(part);
					}		
				}
			}
		}
    }

    void updatePlayerSwingFrame(int frame) {
        PlayerObject::updatePlayerSwingFrame(frame);
		updatePlayerShaders();
    }

    void updatePlayerJetpackFrame(int frame) {
        PlayerObject::updatePlayerJetpackFrame(frame);
		updatePlayerShaders();
    }
};


class GlobedSimplePlayer : public CCNode {};

class $modify(MyPlayLayer, PlayLayer) {

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		schedule(schedule_selector(MyPlayLayer::checkGlobed));
		return true;
	}

    void showCompleteEffect() {
		PlayLayer::showCompleteEffect();
		if (m_player1) static_cast<MyPlayerObject*>(m_player1)->setOutlineSpriteOpacity(0);
		if (m_player2) static_cast<MyPlayerObject*>(m_player2)->setOutlineSpriteOpacity(0);
	}

	void levelComplete() {
		PlayLayer::levelComplete();
		if (m_player1) static_cast<MyPlayerObject*>(m_player1)->setOutlineSpriteOpacity(0);
		if (m_player2) static_cast<MyPlayerObject*>(m_player2)->setOutlineSpriteOpacity(0);
	}

    void resetLevelFromStart() {

		PlayLayer::resetLevelFromStart();
		if (m_player1) static_cast<MyPlayerObject*>(m_player1)->setOutlineSpriteOpacity(255);
		if (m_player2) static_cast<MyPlayerObject*>(m_player2)->setOutlineSpriteOpacity(255);
	}

	void checkGlobed(float dt) {
		if (CCNode* wrapper = m_progressBar->getChildByID("dankmeme.globed2/progress-bar-wrapper")) {
			if (CCNode* progressIcon = wrapper->getChildByID("dankmeme.globed2/self-player-progress")) {
				if (GlobedSimplePlayer* globedSimplePlayer = getChildOfType<GlobedSimplePlayer>(progressIcon, 0)) {
					if (SimplePlayer* player = getChildOfType<SimplePlayer>(globedSimplePlayer, 0)) {
						static_cast<MySimplePlayer*>(player)->updatePlayerShaders();
						unschedule(schedule_selector(MyPlayLayer::checkGlobed));
					}
				}
			}
		}
	}
};

class $modify(MyLevelEditorLayer, LevelEditorLayer) {

	void onPlaytest() {

		LevelEditorLayer::onPlaytest();
		if (m_player1) static_cast<MyPlayerObject*>(m_player1)->setOutlineSpriteOpacity(255);
		if (m_player2) static_cast<MyPlayerObject*>(m_player2)->setOutlineSpriteOpacity(255);
	}
};

class $modify(MyProfilePage, ProfilePage) {

    void getUserInfoFinished(GJUserScore* p0) {
		ProfilePage::getUserInfoFinished(p0);

		if (m_ownProfile) {
			CCNode* playerMenu = m_mainLayer->getChildByID("player-menu");
			for (CCNode* menuChild : CCArrayExt<CCNode*>(playerMenu->getChildren())) {
				if (SimplePlayer* player = getChildOfType<SimplePlayer>(menuChild, 0)) {
					static_cast<MySimplePlayer*>(player)->updatePlayerShaders();
				}
				if (CCNode* innerNode = getChildOfType<CCNode>(menuChild, 0)) {
					if (SimplePlayer* player = getChildOfType<SimplePlayer>(innerNode, 0)) {
						static_cast<MySimplePlayer*>(player)->updatePlayerShaders();
					}
				}
			}
		}
	}

    void toggleShip(CCObject* sender) {
		ProfilePage::toggleShip(sender);
		if (m_ownProfile) {
			CCNode* playerMenu = m_mainLayer->getChildByID("player-menu");
			if (CCNode* shipNode = playerMenu->getChildByID("player-ship")) {
				if (SimplePlayer* player = getChildOfType<SimplePlayer>(shipNode, 0)) {
					static_cast<MySimplePlayer*>(player)->updatePlayerShaders();
				}
			}
		}
	}

};

class $modify(MyGJGarageLayer, GJGarageLayer) {

	static void onModify(auto& self) {
        (void)self.setHookPriority("GJGarageLayer::init", -2);
		(void)self.setHookPriority("GJGarageLayer::onSelect", -2);

    }

    bool init() {
		if (!GJGarageLayer::init()) return false;

		if (SimplePlayer* player1 = typeinfo_cast<SimplePlayer*>(getChildByID("player-icon"))) {
			static_cast<MySimplePlayer*>(player1)->updatePlayerShaders();
		}

		return true;
	}

    void onSelect(cocos2d::CCObject* sender) {
		GJGarageLayer::onSelect(sender);
		if (SimplePlayer* player1 = typeinfo_cast<SimplePlayer*>(getChildByID("player-icon"))) {
			static_cast<MySimplePlayer*>(player1)->updatePlayerShaders();
		}
	}

};

class OutlineColorPickPopupDelegate : public ColorPickPopupDelegate {
    
	Ref<CCArray> m_icons;
	public:
	void init(CCArray* icons) {
		m_icons = icons;
	}

	void updateColor(ccColor4B const& c) {

		for (CCNode* children : CCArrayExt<CCNode*>(m_icons)) {
			if (SimplePlayer* player = typeinfo_cast<SimplePlayer*>(children)) {
				static_cast<MySimplePlayer*>(player)->setOutlineColor(ccColor3B{c.r, c.g, c.b});
			}
		}
		Mod::get()->setSavedValue<ccColor3B>("p1-color", ccColor3B{c.r, c.g, c.b});

	}
};

class $modify(MyCharacterColorPage, CharacterColorPage) {

	static void onModify(auto& self) {
        (void)self.setHookPriority("CharacterColorPage::init", -2);
        (void)self.setHookPriority("CharacterColorPage::toggleShip", -2);
    }

	struct Fields {
		CCMenuItemSpriteExtra* m_outlineColorBtn;
		OutlineColorPickPopupDelegate* m_outlineColorDelegate;

		~Fields() {
			delete m_outlineColorDelegate;
		}
	};

    bool init() {

		if (!CharacterColorPage::init()) return false;

 		m_fields->m_outlineColorDelegate = new OutlineColorPickPopupDelegate();

		for (CCNode* children : CCArrayExt<CCNode*>(m_playerObjects)) {
			if (SimplePlayer* player = typeinfo_cast<SimplePlayer*>(children)) {
				static_cast<MySimplePlayer*>(player)->updatePlayerShaders();
			}
		}

		CCSprite* outlineColorSpr = CCSprite::createWithSpriteFrameName("GJ_paintBtn_001.png");
		outlineColorSpr->setScale(0.5);

		m_fields->m_outlineColorBtn = CCMenuItemSpriteExtra::create(outlineColorSpr, this, menu_selector(MyCharacterColorPage::onOutlineColor));
		m_fields->m_outlineColorBtn->setVisible(false);
		if (CCMenu* buttonsMenu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByID("buttons-menu"))) {

			m_fields->m_outlineColorBtn->setPosition({m_glowToggler->getPosition().x + 74, m_glowToggler->getPosition().y});
			buttonsMenu->addChild(m_fields->m_outlineColorBtn);
			if (CCMenuItemSpriteExtra* closeButton = typeinfo_cast<CCMenuItemSpriteExtra*>(buttonsMenu->getChildByID("close-button"))) {
				closeButton->m_pfnSelector = menu_selector(MyCharacterColorPage::onCloseH);
			}
		}
		
		return true;
	}

	void onOutlineColor(CCObject* sender) {
		geode::ColorPickPopup* colorPopup = geode::ColorPickPopup::create(Mod::get()->getSavedValue<ccColor3B>("p1-color"));
		m_fields->m_outlineColorDelegate->init(m_playerObjects);
		colorPopup->setDelegate(m_fields->m_outlineColorDelegate);
		colorPopup->show();
	}

    void onMode(CCObject* sender) {
		CharacterColorPage::onMode(sender);
		m_fields->m_outlineColorBtn->setVisible(m_colorMode == 2);
	}


    void toggleShip(CCObject* sender) {
		CharacterColorPage::toggleShip(sender);
		for (CCNode* children : CCArrayExt<CCNode*>(m_playerObjects)) {
			if (children->getParent()->getID() == "ship-button") {
				if (SimplePlayer* player = typeinfo_cast<SimplePlayer*>(children)) {
					static_cast<MySimplePlayer*>(player)->updatePlayerShaders();
				}
				return;
			}
		}
	}

	void setColorOnGarage() {
		CCScene* scene = CCDirector::get()->m_pRunningScene;
		if (GJGarageLayer* garage = getChildOfType<GJGarageLayer>(scene, 0)) {
			if (SimplePlayer* player1 = typeinfo_cast<SimplePlayer*>(garage->getChildByID("player-icon"))) {
				static_cast<MySimplePlayer*>(player1)->setOutlineColor(Mod::get()->getSavedValue<ccColor3B>("p1-color"));
			}
		}
	}

	#ifndef GEODE_IS_ANDROID
    void keyBackClicked() {
		CharacterColorPage::keyBackClicked();
		setColorOnGarage();
	}
	#endif

    void onCloseH(CCObject* sender) {
		CharacterColorPage::onClose(sender);
		setColorOnGarage();
	}
};