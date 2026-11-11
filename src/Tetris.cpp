#include "Tetris.h"

#include <ios>

using namespace TetrisConst;
using namespace NoteCodes;

void Tetris::init(GameContext& context, Renderer& renderer){
    SongData themeData = context.resources.readSongFromFile("assets/tetris_theme.trk");
    song = context.audio.buildFullTheme(themeData);
    context.audio.playMusic(song);

    uint8_t numPix;
    uint8_t** spriteMap = context.resources.readSpriteMap("assets/blockTile.tsm", numPix);
    for(uint8_t i = 0; i < 8; i++){
        blocktextures[i] = renderer.makeTextureFromMap8(/*&blockTile[0][0][0]*/spriteMap, border, &colors[i+1], BOX_SIZE, BOX_SIZE, numPix);
    }

    rotateSfx = context.audio.generateTone(G4, 0.5f, 0.1f);
    moveSfx = context.audio.generateTone(G4S*2, 0.3f, 0.08f);;

    SongData fData = context.resources.readSongFromFile("assets/fixSfx.trk");
    fixSfx = context.audio.buildFullTheme(fData);

    SongData cData = context.resources.readSongFromFile("assets/cSfx.trk");
    clearSfx = context.audio.buildFullTheme(cData);

    SongData tetData = context.resources.readSongFromFile("assets/tetSfx.trk");
    tetrisSfx = context.audio.buildFullTheme(tetData);

    SongData lupData = context.resources.readSongFromFile("assets/lupSfx.trk");
    levelupSfx = context.audio.buildFullTheme(lupData);

    SongData goData = context.resources.readSongFromFile("assets/goSfx.trk");
    gameOverSfx = context.audio.buildFullTheme(goData);

    boardRect = {
        (SCREEN_WIDTH/2) - (BOX_SIZE*5),
        PADDING,
        BOX_SIZE*10,
        BOX_SIZE*20
    };

    scoreRect = {
        BOX_SIZE,
        BOX_SIZE *2,
        SCREEN_WIDTH/5,
        64
    };

    linesRect = {
        (SCREEN_WIDTH/5)*4,
        BOX_SIZE *2,
        SCREEN_WIDTH/5,
        64
    };

    borderRect = {
        boardRect.x - PADDING,
        0,
        BOX_SIZE * 10 + PADDING*2,
        SCREEN_HEIGHT
    };

    nextRect = {
        borderRect.x - (BOX_SIZE*5),
        (SCREEN_HEIGHT*0.5f) - (BOX_SIZE*2),
        BOX_SIZE*4,
        BOX_SIZE*4
    };

    nextBorderRect = {
        nextRect.x - BOX_SIZE,
        nextRect.y - BOX_SIZE,
        BOX_SIZE*6,
        BOX_SIZE*6
    };

    holdRect = {
        borderRect.x + borderRect.width + BOX_SIZE,
        (SCREEN_HEIGHT*0.5f) - (BOX_SIZE*2),
        BOX_SIZE*4,
        BOX_SIZE*4
    };
    holdBorderRect = {
        holdRect.x - BOX_SIZE,
        holdRect.y - BOX_SIZE,
        BOX_SIZE*6,
        BOX_SIZE*6
    };

    lockTimer = 0;

    createBorderTexture(renderer);

    Font promptFont("assets/pong_font.ttf", 28);
    nextWrite = promptFont.createTextTexture(renderer, "NEXT", ColorCodes::WHITE);
    holdWrite = promptFont.createTextTexture(renderer, "HOLD", ColorCodes::WHITE);

    scoreTex = promptFont.createTextTexture(renderer, "SCORE", ColorCodes::WHITE);
    lvlTex = promptFont.createTextTexture(renderer, "LEVEL", ColorCodes::WHITE);

    nextTextRect = {
        nextBorderRect.x,
        nextBorderRect.y - BOX_SIZE,
        nextBorderRect.width,
        BOX_SIZE
    };

    holdTextRect = {
        holdBorderRect.x,
        holdBorderRect.y - BOX_SIZE,
        holdBorderRect.width,
        BOX_SIZE
    };

    isGridOn = false;

    /*!!!*/
    level = 0;
    score = 0;
    lines = LINES;

    setDropSpeedMS();

    fillBag();
    pieceType = bag.at(bagIndex++);
    nextPieceType = bag.at(bagIndex++);
    newPiece(context.audio);

    currentState = TetrisState::Playing;
}

void Tetris::update(float deltaTime, GameContext& context){
    switch(currentState){
        case TetrisState::Playing:
            if(spawnDelayTimer > 0.0f){
                spawnDelayTimer -= deltaTime;
                return;
            }

            if(context.input.wasKeyPressed(Key::Q)){
                rotateSRS(false, context.audio);
            }
            if(context.input.wasKeyPressed(Key::W)){
                while(!checkCollision(r)){
                    r++;
                }
                r--;
                fix(context.audio);
            }
            if(context.input.wasKeyDAS(Key::E, DAS_DELAY, RAPID_REPEAT)){
                rotateSRS(true, context.audio);
            }
            if(context.input.wasKeyDAS(Key::D, DAS_DELAY, RAPID_REPEAT)){
                moveRight(context.audio);
            }
            if(context.input.wasKeyDAS(Key::S, DAS_DELAY, RAPID_REPEAT)){
                moveDown(context.audio);
                gravityTimer = currGravitySpeed;
            }
            if(context.input.wasKeyDAS(Key::A, DAS_DELAY, RAPID_REPEAT)){
                moveLeft(context.audio);
            }
            if(context.input.wasKeyPressed(Key::G)){
                //isGridOn = (isGridOn) ? false : true;
                swapPiece(context.audio);
            }
            if(context.input.wasKeyPressed(Key::Left)){
                rotateSRS(false, context.audio);
            }
            if(context.input.wasKeyPressed(Key::Right)){
                rotateSRS(true, context.audio);
            }

            if(isDown && lockTimer <= 0.0f){
                fix(context.audio);
            }else if(lockTimer > 0.0f){
                lockTimer -= deltaTime;
            }else{
                gravityTimer -= deltaTime;
                if(gravityTimer <= 0.0f){
                    moveDown(context.audio);
                    gravityTimer = currGravitySpeed;
                }
            }
            break;
        case TetrisState::GameOverAnim:
            animTimer -= deltaTime;
            if(animTimer <= 0.0f){
                for(uint8_t col = 1; col < BOARD_WIDTH - 1; col++){
                    if(board[animRow][col] > 0){
                        board[animRow][col] = 8;
                    }
                }
                animRow--;
                animTimer = 0.2f;
                if(animRow < -2){
                    context.scenes.pushScene(std::make_unique<MainMenu>());
                }
            }
            break;
        case TetrisState::Flashing:
            flashTimer -= deltaTime;
            if(flashTimer <= 0.0f){
                for(uint8_t line : flashingLines){
                    deleteLine(line, context.audio);
                }
                currentState = TetrisState::Playing;
                newPiece(context.audio);
            }
            break;
    }
    if(context.input.wasKeyPressed(Key::P)){
        context.scenes.pushScene(std::make_unique<PauseScene>());
    }
    if(context.input.wasKeyPressed(Key::Escape)){
        context.scenes.changeScene(std::make_unique<MainMenu>());
    }
}

void Tetris::render(Renderer& renderer){
    renderer.setColor(ColorCodes::BLACK);
    renderer.clear();

    renderer.setColor(ColorCodes::WHITE);
    renderer.drawScore(scoreRect, score, 7);
    renderer.drawScore(linesRect, level, 3);

    renderPiece(renderer, centerPiece(nextRect, nextPieceType), blocktextures[nextPieceType], nextPiece);
    if(holding){
        renderPiece(renderer, centerPiece(holdRect, holdPieceType), blocktextures[holdPieceType], holdPiece);
    }

    renderer.drawTexture(nextTexture[level % 3], nextBorderRect);
    renderer.drawTexture(nextWrite, nextTextRect);

    renderer.drawTexture(nextTexture[level % 3], holdBorderRect);
    renderer.drawTexture(holdWrite, holdTextRect);

    renderer.drawTexture(borderTexture[level % 3], borderRect);
    renderBoard(renderer);

    renderer.drawTexture(scoreTex, {scoreRect.x, 0, scoreRect.width, BOX_SIZE});
    renderer.drawTexture(lvlTex, {linesRect.x, 0, linesRect.width, BOX_SIZE});

    if(currentState == TetrisState::Flashing){
        Color color = (flashTimer < 0.1f || flashTimer > 0.2f) ? ColorCodes::WHITE : ColorCodes::BLACK;
        renderer.setColor(color);
        for(uint8_t line : flashingLines){
            renderer.drawRect({boardRect.x, boardRect.y + (BOX_SIZE*((line)-1)), boardRect.width, BOX_SIZE}, true);
        }
    }else if(currentState != TetrisState::GameOverAnim){
        renderOutline(renderer, currPiece);
        renderCurrPiece(renderer, blocktextures[pieceType]);
    }
}

void Tetris::moveRight(AudioSystem& audio){
    c++;
    if(checkCollision(r)){
        c--;
        return;
    }
    audio.playSfx(moveSfx);

    lockTimer = LOCK_DELAY;
    if(!checkCollision(r+1)){
        isDown = false;
        lockTimer = 0.0f;
    }
}

void Tetris::moveLeft(AudioSystem& audio){
    c--;
    if(checkCollision(r)){
        c++;
        return;
    }
    audio.playSfx(moveSfx);

    lockTimer = LOCK_DELAY;

    // Check if the piece fell off a ledge
    if(!checkCollision(r + 1)){
        isDown = false;
        lockTimer = 0.0f;
    }
}

void Tetris::moveDown(AudioSystem& audio){
    r++;
    if(checkCollision(r)){
        r--;
        lockTimer = LOCK_DELAY;
        isDown = true;
        //fix(audio);
    }
}

void Tetris::setPiece(){
    pieceType = (pieceType % 7);
    currPiece = TETROMINOES[pieceType][currRot];
    nextPieceType = (nextPieceType % 7);
    nextPiece = TETROMINOES[nextPieceType][0];
}

void Tetris::rotateSRS(bool clockwise, AudioSystem& audio){
    uint8_t oldRot = currRot;

    uint8_t nextRot = clockwise ? (currRot + 1) % 4 : (currRot + 3) % 4;
    uint8_t dirIndex = clockwise ? 1 : 0;

    if(pieceType == 3){
        currRot = nextRot;
        setPiece();
        if(checkCollision(r)){
            currRot = clockwise ? (nextRot - 1 + 4) % 4 : (nextRot + 1) % 4;
            setPiece();
            audio.playSfx(rotateSfx);
        }else{
            lockTimer = LOCK_DELAY;
            if(!checkCollision(r + 1)){
                isDown = false;
                lockTimer = 0.0f;
            }
        }
        return;
    }

    for(uint8_t test = 0; test < 5; test++){
        int8_t offsetX;
        int8_t offsetY;

        if(!pieceType){
            offsetX = KICK_TABLE_I[oldRot][dirIndex][test][0];
            offsetY = KICK_TABLE_I[oldRot][dirIndex][test][1];
        }else{
            offsetX = KICK_TABLE[oldRot][dirIndex][test][0];
            offsetY = KICK_TABLE[oldRot][dirIndex][test][1];
        }


        c += offsetX;
        r -= offsetY;
        currRot = nextRot;
        setPiece();

        if(!checkCollision(r)){
            audio.playSfx(rotateSfx);

            lockTimer = LOCK_DELAY;

            if(!checkCollision(r + 1)){
                isDown = false;
                lockTimer = 0.0f;
            }
            return;
        }

        c -= offsetX;
        r += offsetY;
    }

    currRot = oldRot;
    setPiece();
}

void Tetris::createBorderTexture(Renderer& renderer){
    Color cols[3] = { ColorCodes::BRONZE, ColorCodes::SILVER, ColorCodes::GOLD };
    float w = (BOX_SIZE*10 + PADDING*2);
    float nb = border*2;
    for(uint8_t i = 0; i < 3; i++){
        borderTexture[i] = renderer.createRenderTarget(w, SCREEN_HEIGHT);
        renderer.setRenderTarget(&borderTexture[i]);

        renderer.setColor(0, 0, 0, 0);
        renderer.clear();

        renderer.setColor(cols[i]);
        renderer.drawRect({0, 0, PADDING, SCREEN_HEIGHT}, true);            // LEFT
        renderer.drawRect({0, 0, w, PADDING}, true);                        // TOP
        renderer.drawRect({w - PADDING, 0, PADDING, SCREEN_HEIGHT}, true);  // RIGHT
        renderer.drawRect({0, BOX_SIZE*20 + PADDING, w, PADDING}, true);    // BOTTOM

        renderer.setColor(calculateHighlight(cols[i]));
        renderer.drawRect({0, 0, nb, SCREEN_HEIGHT}, true);            // LEFT
        renderer.drawRect({0, 0, w, nb}, true);                        // TOP
        renderer.drawRect({w - PADDING, PADDING - nb, nb, SCREEN_HEIGHT - PADDING*2 + nb}, true);  // RIGHT
        renderer.drawRect({PADDING - nb, BOX_SIZE*20 + PADDING, BOX_SIZE*10 + nb*2, nb}, true);    // BOTTOM

        renderer.setColor(calculateShadow(cols[i]));
        renderer.drawRect({PADDING - nb, PADDING, nb, SCREEN_HEIGHT - PADDING*2}, true);            // LEFT
        renderer.drawRect({PADDING - nb, PADDING - nb, BOX_SIZE*10 + nb*2, nb}, true);                        // TOP
        renderer.drawRect({w - nb, 0, nb, SCREEN_HEIGHT}, true);  // RIGHT
        renderer.drawRect({0, SCREEN_HEIGHT - nb, w, nb}, true);    // BOTTOM

        renderer.setRenderTarget(nullptr);
        nextTexture[i] = renderer.createRenderTarget(nextBorderRect.width, nextBorderRect.height);
        renderer.setRenderTarget(&nextTexture[i]);

        renderer.setColor(0, 0, 0, 0);
        renderer.clear();

        renderer.setColor(cols[i]);
        renderer.drawRect({0, 0, BOX_SIZE, nextBorderRect.height}, true);            // LEFT
        renderer.drawRect({0, 0, nextBorderRect.width, BOX_SIZE}, true);                        // TOP
        renderer.drawRect({BOX_SIZE*5, 0, BOX_SIZE, nextBorderRect.height}, true);  // RIGHT
        renderer.drawRect({0, BOX_SIZE*5, nextBorderRect.width, BOX_SIZE}, true);    // BOTTOM

        renderer.setColor(calculateHighlight(cols[i]));
        renderer.drawRect({0, 0, nb, nextBorderRect.height}, true);            // LEFT
        renderer.drawRect({0, 0, nextBorderRect.width, nb}, true);                        // TOP
        renderer.drawRect({BOX_SIZE*5, BOX_SIZE, nb, nextBorderRect.height - BOX_SIZE*2 + nb}, true);  // RIGHT
        renderer.drawRect({BOX_SIZE - nb, BOX_SIZE*5, BOX_SIZE*4 + nb*2, nb}, true);    // BOTTOM

        renderer.setColor(calculateShadow(cols[i]));
        renderer.drawRect({BOX_SIZE - nb, BOX_SIZE, nb, nextBorderRect.height - BOX_SIZE*2}, true);            // LEFT
        renderer.drawRect({BOX_SIZE - nb, BOX_SIZE - nb, BOX_SIZE*4 + nb*2, nb}, true);                        // TOP
        renderer.drawRect({nextBorderRect.width - nb, 0, nb, nextBorderRect.height}, true);  // RIGHT
        renderer.drawRect({0, nextBorderRect.height - nb, nextBorderRect.width, nb}, true);    // BOTTOM
    }

    renderer.setRenderTarget(nullptr);
}

void Tetris::renderPiece(Renderer& renderer, const Rect& dest, Texture& blocktTex, uint16_t piece){
    for(int pos = 0; piece > 0; pos++, piece <<=1){
        if(piece & 0x8000){
            int x = pos%4;
            int y = pos/4;

            Rect newDest = {
                static_cast<float>(dest.x + (x*BOX_SIZE)),
                static_cast<float>(dest.y + (y*BOX_SIZE)),
                BOX_SIZE,
                BOX_SIZE
            };

            renderer.drawTexture(blocktTex, newDest);
        }
    }
}

void Tetris::renderOutline(Renderer& renderer, uint16_t piece){
    int8_t dummyR = r;
    while( !checkCollision(dummyR) ) dummyR++;
    dummyR--;

    float startX = boardRect.x + (BOX_SIZE*(c-1));
    float startY = boardRect.y + (BOX_SIZE*(dummyR-1));

    for(int pos = 0; piece > 0; pos++, piece <<=1){
        if(piece & 0x8000){
            int x = pos%4;
            int y = pos/4;

            Rect newDest = {
                static_cast<float>(startX + (x*BOX_SIZE)),
                static_cast<float>(startY + (y*BOX_SIZE)),
                BOX_SIZE,
                BOX_SIZE
            };

            renderer.setColor(colors[pieceType+1]);
            renderer.drawRect(newDest, false);
        }
    }
}

void Tetris::renderCurrPiece(Renderer& renderer, Texture& blockTex){
    float startX = boardRect.x + (BOX_SIZE*(c-1));
    float startY = boardRect.y + (BOX_SIZE*(r-1));

    renderPiece(renderer, {startX, startY, BOX_SIZE*4, BOX_SIZE*4}, blockTex, currPiece);
}

void Tetris::renderBoard(Renderer& renderer){
    if(isGridOn){
        renderer.setColor(ColorCodes::WHITE);
        renderer.drawGrid(boardRect, ROWS, COLS);
    }

    for(int i = 1; i < BOARD_HEIGHT - 1 ; i++){
        for(int j = 1; j < BOARD_WIDTH - 1; j++){
            int blockType = board[i][j];
            if(blockType > 0){
                Rect cell = {boardRect.x + ((j-1)*BOX_SIZE), boardRect.y + ((i-1)*BOX_SIZE), BOX_SIZE, BOX_SIZE};
                renderer.drawTexture(blocktextures[blockType-1], cell);
            }
        }
    }
}

bool Tetris::checkCollision(int8_t destRow) const{
    for(int row = 0; row < 4; row++){
        for(int col = 0; col < 4; col++){
            if((currPiece >> (15 - ((row*4)+col))) & 1){
                int8_t checkRow = destRow + row;
                int8_t checkCol = c + col;

                if(checkRow < 0 || checkRow >= BOARD_HEIGHT ||
                   checkCol < 0 || checkCol >= BOARD_WIDTH){
                    return true;
                }

                if(board[checkRow][checkCol]) return true;
            }
        }
    }
    return false;
}

void Tetris::fix(AudioSystem& audio){
    for(uint8_t row = 0; row < 4; row++){
        for(uint8_t col = 0; col < 4; col++){
            if((currPiece >> (15 - ((row*4)+col))) & 1){
                uint8_t boardRow = r + row;
                uint8_t boardCol = c + col;

                if(boardRow >= 0 && boardRow < BOARD_HEIGHT &&
                   boardCol >= 0 && boardCol < BOARD_WIDTH){
                    board[r+row][c+col] = pieceType + 1;
                }
            }
        }
    }
    //uint8_t deletedCOunt = 0;
    flashingLines.clear();
    for(uint8_t row = 0; row < 4; row++){
        uint8_t boardRow = r + row;
        if(boardRow >= BOARD_HEIGHT - 1) continue;
        if(checkLine(boardRow)){
            //deleteLine(boardRow, audio);
            flashingLines.push_back(boardRow);
            //deletedCOunt++;
        }
    }

    if(!flashingLines.empty()){
        increaseScore(flashingLines.size());
        currentState = TetrisState::Flashing;
        flashTimer = FLASH_TIMER;
        if(flashingLines.size() == 4){
            audio.playSfx(tetrisSfx);
        }else{
            audio.playSfx(clearSfx);
        }
    }else{
        audio.playSfx(fixSfx);
        newPiece(audio);
    }
}

bool Tetris::checkLine(uint8_t line) const{
    for(uint8_t i = 1; i < BOARD_WIDTH - 1; i++){
        if(!board[line][i]) return false;
    }
    return true;
}

void Tetris::deleteLine(uint8_t line, AudioSystem& audio){
    size_t bytesToMove = (line - 1) * BOARD_WIDTH;
    memmove(&board[2][0], &board[1][0], bytesToMove);
    for(int i = 1; i < BOARD_WIDTH - 1; i++){
        board[1][i] = 0;
    }
    lines--;
    if(!lines){
        levelUp(audio);
    }
}

void Tetris::newPiece(AudioSystem& audio){
    spawnDelayTimer = SPAWN_DELAY;

    pieceType = nextPieceType;

    if(bagIndex >= 7){
        fillBag();
    }

    nextPieceType = bag.at(bagIndex);
    bagIndex++;

    r = 1;
    c = 4;
    currRot = 0;
    setPiece();

    isDown = false;

    if(checkCollision(r)){
        currentState = TetrisState::GameOverAnim;
        audio.stopMusic();
        audio.playSfx(gameOverSfx);
    }

    if(justSwapped) justSwapped = false;
}

void Tetris::swapPiece(AudioSystem& audio){
    if(!holding){
        holdPieceType = pieceType;
        holdPiece = TETROMINOES[holdPieceType][0];
        justSwapped = true;
        holding = true;
        newPiece(audio);
        return;
    }

    if(!justSwapped){
        r = 1;
        c = 4;
        currRot = 0;

        uint8_t tempPieceType = holdPieceType;
        uint16_t tempPiece = TETROMINOES[tempPieceType][0];

        holdPieceType = pieceType;
        holdPiece = TETROMINOES[pieceType][0];

        pieceType = tempPieceType;
        currPiece = tempPiece;

        spawnDelayTimer = SPAWN_DELAY;

        justSwapped = true;
        holding = true;

        isDown = false;
        if(checkCollision(r)){
            currentState = TetrisState::GameOverAnim;
            audio.stopMusic();
            audio.playSfx(gameOverSfx);
        }
    }
}

void Tetris::setDropSpeedMS(){
    uint8_t frame = 2;
    if(level >= 29){
        frame = 1;
    }else if(level < 20){
        frame = FRAMES[level];
    }

    float sPerFrame = 1.0f / 60.0f;
    currGravitySpeed = static_cast<float>(frame)*sPerFrame;
    gravityTimer = currGravitySpeed;
}

void Tetris::increaseScore(uint8_t lines){
    score += (POINTS[lines-1]) * (level+1);
}

void Tetris::levelUp(AudioSystem& audio){
    lines = LINES;
    level++;
    setDropSpeedMS();
    audio.playSfx(levelupSfx);
}

void Tetris::fillBag(){
    std::iota(bag.begin(), bag.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(bag.begin(), bag.end(), g);

    bagIndex = 0;
}

Rect Tetris::centerPiece(Rect pieceRect, uint8_t type){
    Rect newDest = {
        pieceRect.x,
        pieceRect.y,
        pieceRect.width,
        pieceRect.height
    };

    if( type == 0 /*I*/ ){
        newDest.y += (BOX_SIZE*0.5f);
    }else if( type == 1 /*J*/ ){
        newDest.x += BOX_SIZE;
        newDest.y += (BOX_SIZE*0.5f);
    }else if( type == 2 /*L*/ ){
        newDest.y += (BOX_SIZE*0.5f);
    }else if( type == 3 /*O*/){
        newDest.x += BOX_SIZE;
        newDest.y += BOX_SIZE;
    }else if( type == 4 /*S*/ || type == 6 /*Z*/ || type == 5 /*T*/ ){
        newDest.x += (BOX_SIZE*0.5f);
    }

    return newDest;
}
