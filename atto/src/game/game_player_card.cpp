#include "game_player_card.h"


namespace atto {
    void PlayerCard::Initialize() {
        Renderer & renderer = Engine::Get().GetRenderer();
        front = renderer.GetOrLoadTexture( "assets/textures/cards/front.png", true );
        emptyGem =  renderer.GetOrLoadTexture( "assets/textures/cards/empty-gem.png", true );
        fullGem =  renderer.GetOrLoadTexture( "assets/textures/cards/full-gem.png", true );
    }


}

