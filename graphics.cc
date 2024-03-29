#include "application.h"
void application::clearDrawLists() {
	drawListTrue.clear();
	drawListFalse.clear();
}

void application::addRectangle( bool bitState, SDL_Rect footprint ) {
	if ( bitState ) {
		drawListTrue.push_back( footprint );
	} else {
		drawListFalse.push_back( footprint );
	}
	// std::cout << "adding rectangle at " << footprint.x << " " << footprint.y << " with size " << footprint.w << " " << footprint.h << std::endl;
}

void application::updateRegisters() {
	// program counter ( single 16-bit value )
	for ( int i = 0; i < 16; i++ ) {
		SDL_Rect drawPosition = { go.PCBaseX + i * ( go.PCBoxDim + go.PCOffset ),
			go.PCBaseY, go.PCBoxDim, go.PCBoxDim };
		addRectangle( bool( programCounter >> ( 15 - i ) & 1 ), drawPosition );
	}

	// v registers ( 16x 8-bit values )
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = 0; j < 8; j++ ) {
			SDL_Rect drawPosition = { go.RBaseX + j * ( go.RBoxDim + go.ROffset ),
				go.RBaseY + i * ( go.RBoxDim + go.ROffset ), go.RBoxDim, go.RBoxDim };
			addRectangle( bool( vregisters[ i ] >> ( 7 - j ) & 1 ), drawPosition );
		}
	}

	// delay + sound timer ( 2x 8-bit values )
	for ( int i = 0; i < 8; i++ ) {
		SDL_Rect drawPosition = { go.TBaseX + i * ( go.TBoxDim + go.TOffset ),
			go.TBaseY, go.TBoxDim, go.TBoxDim };
		addRectangle( bool( delayTimer >> ( 7 - i ) & 1 ), drawPosition );
		drawPosition.y += ( go.TBoxDim + go.TOffset );
		addRectangle( bool( soundTimer >> ( 7 - i ) & 1 ), drawPosition );
	}

	// stack entries
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = 0; j < 16; j++ ) {
			SDL_Rect drawPosition = { go.SBaseX + j * ( go.SBoxDim + go.SOffset ),
				go.SBaseY + i * ( go.SBoxDim + go.SOffset ), go.SBoxDim, go.SBoxDim };
			if ( i <= stackPointer ){
				addRectangle( bool( addressStack[ i ] >> ( 15 - j ) & 1 ), drawPosition );
			} else { // zeroed out if greater than the value of the stack pointer
				addRectangle( false, drawPosition );
			}
		}
	}

	// stack pointer
	for ( int i = 0; i < 8; i++ ) {
		SDL_Rect drawPosition = { go.SPBaseX + i * ( go.SPBoxDim + go.SPOffset ),
			go.SPBaseY, go.SPBoxDim, go.SPBoxDim };
		addRectangle( bool( stackPointer >> ( 7 - i ) & 1 ), drawPosition );
	}

	// input state
	for ( int x = 0; x < 4; x++ ) {
		for ( int y = 0; y < 4; y++ ) {
			SDL_Rect drawPosition = { go.IBaseX + x * ( go.IBoxDim + go.IOffsetX ),
				go.IBaseY + y * ( go.IBoxDim + go.IOffsetY ), go.IBoxDim, go.IBoxDim };
			// inputStateSinceLastFrame >> 
			const Uint8* state = SDL_GetKeyboardState(NULL);
			addRectangle( bool( state[ keys[ x + 4 * y ] ] ), drawPosition );
		}
	}
}

void application::updateScreen() {
	// full screen 64x32
	for ( int x = 0; x < bufferWidth; x++ ) {
		for ( int y = 0; y < bufferHeight; y++ ) {
			SDL_Rect drawPosition = { go.SCRBaseX + x * ( go.SCRBoxDim + go.SCROffset ),
				go.SCRBaseY + y * ( go.SCRBoxDim + go.SCROffset ), go.SCRBoxDim, go.SCRBoxDim };
			addRectangle( bool( frameBuffer[ x + y * bufferWidth ] ), drawPosition );
		}
	}
}

void application::updateMemory() {
	// everything 0-4k
	for ( int column = 0; column < 16; column++ ) {
		for ( int row = 0; row < 256; row++ ) {
			for ( int bit = 0; bit < 8; bit++ ) {
				SDL_Rect drawPosition = { go.MBaseX + bit * ( go.MBoxDim + go.MOffset ) + column * go.MColOffset,
					go.MBaseY + row * ( go.MBoxDim + go.MOffset ), go.MBoxDim, go.MBoxDim };
				addRectangle( bool( ram[ row + 256 * column ] >> ( 7 - bit ) & 1 ), drawPosition );
			}
		}
	}
}

void application::drawBackground() {
	SDL_Rect drawLocation = { 0, 0, windowWidth, windowHeight };
	SDL_RenderCopyEx( renderer, texture, NULL, &drawLocation, 0.0f, NULL, SDL_FLIP_NONE );
}

void application::updateGraphicsFull() {
	// for intial clearing of buffer
	drawBackground();
	clearDrawLists();
	updateRegisters();
	updateScreen();
	updateMemory();

	// true bits
	SDL_SetRenderDrawColor( renderer, 255, 0, 0, 255 );
	SDL_RenderFillRects( renderer, &drawListTrue[ 0 ], drawListTrue.size() );

	// false bits
	SDL_SetRenderDrawColor( renderer, 50, 50, 50, 255 );
	SDL_RenderFillRects( renderer, &drawListFalse[ 0 ], drawListFalse.size() );

	// present on the renderer
	SDL_RenderPresent( renderer );
}

bool application::update() {
	updateGraphicsFull();

	static int prevTicks = 0;
	int currTicks = SDL_GetTicks();
	int timeSinceLastPass = currTicks - prevTicks;
	if( timeSinceLastPass < 16 )
		SDL_Delay( 16 - timeSinceLastPass );

	std::cout << "\rTime since last frame: " << SDL_GetTicks() - prevTicks << "ms  " << std::flush;
	prevTicks = currTicks;

	// doing the timer decrement here ensures 60hz update
	if ( soundTimer > 0 ) {
		playSound();
		soundTimer--;
	}

	if ( delayTimer > 0 ) {
		delayTimer--;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE) {
			return false; // terminate on release of escape key
		}
	}
	tick();
	return true;
}
