/* code reference
 * Dr. Freudenthal demos
 */
#include <msp430.h>
#include <abCircle.h>
#include <shape.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <libTimer.h>
#include "buzzer.h"
#include "scoreSet.h"
#define GREEN_LED BIT6

int paddleLoc = 0; //location of the paddle
int button;
int endOfGame = 0; //1 when the game ends, clears the screen
int screenOn = 0; //screen display
char score = '0'; //keep count of score
long count = 0;

AbRect paddleShape = {abRectGetBounds, abRectCheck, {22, 4}}; //22x4 rectangle
AbRectOutline playField = {abRectOutlineGetBounds, abRectOutlineCheck,  //playing field for the game
			   {screenWidth/2-1, screenHeight/2-1}};

Layer base = {(AbShape *)&playField, {screenWidth/2, screenHeight/2}, //main screen layer
	      {0, 0}, {0, 0}, COLOR_KHAKI, 0};
Layer playLayer = {(AbShape *)&playField, {screenWidth/2, screenHeight/2}, //playing field set to layer
		   {0, 0}, {0, 0}, COLOR_ORANGE, 0};
Layer pad = {(AbShape *)&paddleShape, {screenWidth/2, 156}, //paddle set to a layer
	     {0, 0}, {0, 0}, COLOR_DARK_GREEN, &playLayer};
Layer ball = {(AbShape *)&circle2, {screenWidth/2, screenHeight/2}, //sets the ball at the center
	      {0, 0}, {0, 0}, COLOR_VIOLET, &pad};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s{
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

MovLayer paddleMove = {&pad, {2, 2}, 0};
MovLayer ballMove = {&ball, {3, 3}, 0};

void movLayerDraw(MovLayer *movLayers, Layer *layers){
  int row, col;
  MovLayer *movLayer;

  and_sr(~8); /**< disable interrupts (GIE off)*/
  for(movLayer = movLayers; movLayer; movLayer = movLayer->next)
    {
      Layer *l = movLayer->layer;
      l->posLast = l->pos;
      l->pos = l->posNext;
    }
  or_sr(8); /**<disable interrupts (GIE on)*/
  for(movLayer = movLayers; movLayer; movLayer = movLayer->next){ /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for(row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++){
      for(col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++){
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for(probeLayer = layers; probeLayer; probeLayer = probeLayer->next){ /* probe all layers, in order */
	  if(abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)){
	    color = probeLayer->color;
	    break;
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color);
      } // for col
    } // for row
  } // for moving layer being updated
}

/** Advances a moving shape within a fence
 *
 * \param m1 The moving shape to be advanced
 * \param fence The region which will serve as a boundary for m1
 */
void m1Advance(MovLayer *m1, Region *fence){
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; m1; m1 = m1->next){
    vec2Add(&newPos, &m1->layer->posNext, &m1->velocity);
    abShapeGetBounds(m1->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++){
      if((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])){
	playLayer.pos.axes[1] += 10; //For checking the bottom fence.
	//if ball hits bottom fence...
	if(abRectCheck(&playField, &(playLayer.pos), &(m1->layer->pos)) && axis == 1){
	  screenOn = 2;
	} else{
	  int velocity = m1->velocity.axes[axis] = -m1->velocity.axes[axis];
	  newPos.axes[axis] += (2*velocity);
	  buzzer_set_period(1800);
	  while(++count < 25000){}
	  buzzer_set_period(0);
	  count - 0;
	}
	playLayer.pos.axes[1] -= 10;
	//if ball hits paddle...
	if(abRectCheck(&paddleShape, &(pad.pos), &(m1->layer->pos)) && axis == 1){
	  int velocity = m1->velocity.axes[axis] = -m1->velocity.axes[axis];
	  newPos.axes[axis] += (2*velocity);
	  score = addScore();
	  screenOn = 1; //stay on screen
	  //play sound on hit
	  buzzer_set_period(3500);
	  while(++count < 25000){}
	  buzzer_set_period(0);
	  count = 0;
	}
      } /**< if outside of fence */
    } /**< for axis */
    m1->layer->posNext = newPos; //sets new position to m1
  }
}

  void startScreen()
  {
    drawString5x7(40, 3, "PaddleBall", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(47, 30, "Press", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(37, 50, "Button 2", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(43, 70, "to Begin", COLOR_BLACK, COLOR_BLUE);
    if(!(BIT1 & button))
      {
	clearScreen(COLOR_BLUE);
	layerDraw(&ball);
	screenOn = 1;
      }
  }

  void gameEnd()
  {
    if(endOfGame == 0)
      {
	buzzer_set_period(8000);
	while(++count < 500000){}
	buzzer_set_period(0);
	count = 0;
	clearScreen(COLOR_BLUE);
	endOfGame++;
      }
    drawString5x7(40, 2, "GAME OVER", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(43, 17, "Score:", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(81, 17, &score, COLOR_BLACK, COLOR_BLUE);
    drawString5x7(41, screenHeight/2, "Press a", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(42, (screenHeight/2)+15, "button to", COLOR_BLACK, COLOR_BLUE);
    drawString5x7(52, (screenHeight/2)+30, "try again!", COLOR_BLACK, COLOR_BLUE);
    if(!(button & BIT0) || !(button & BIT1) || !(button & BIT2) || !(button & BIT3))
      {
	clearScreen(0);
	screenOn = 0;
	score = resetScore();
	endOfGame = 0;
	ball.posNext.axes[0] = screenWidth/2;
	ball.posNext.axes[1] = screenHeight/2;
	layerDraw(&base);
      }
  }

  //function that moves the paddle
  void paddleMvmt(int btn)
  {
    paddleMove.layer->posNext.axes[0] = paddleMove.layer->pos.axes[0] + paddleLoc;
    if(!(btn & BIT0))//move left
      {
	if(paddleMove.layer->pos.axes[0] >= 25)
	  {
	    paddleLoc = -10;
	  }
	else
	  {
	    paddleLoc = 0;
	  }
      }
    else //move right
      {
	if(!(btn & BIT3))
	  {
	    if(paddleMove.layer->pos.axes[0] <= 100)
	      {
		paddleLoc = 10;
	      }
	    else
	      {
		paddleLoc = 0;
	      }
	  }
	else
	  {
	    paddleLoc = 0;
	  }
      }
    movLayerDraw(&paddleMove, &pad);
  }

  int screenRedraw = 1; //decides whether or not to redraw the screen
  u_int bgColor = COLOR_BLUE; //background color
  Region boundary; //boundary containing the playing field

  /** Initializes everything, enables interrupts and green LED
   *  and handles the rendering for the screen
   */
  void main(){
    P1DIR |= GREEN_LED; //Green led on when CPU on
    P1OUT |= GREEN_LED;

    configureClocks();
    lcd_init();
    p2sw_init(BIT0+BIT1+BIT2+BIT3);
    buzzerInit();
    clearScreen(COLOR_KHAKI);
    layerInit(&ball);
    layerDraw(&base);
    layerGetBounds(&playLayer, &boundary);

    enableWDTInterrupts(); //enable periodic interrupt
    or_sr(0x8);            //GIE (enable interrupts)

    for(;;)
      {
	button = p2sw_read();
	if(screenOn == 0)
	  startScreen();
	else if(screenOn == 2)
	  gameEnd();
	else
	  {
	    while(!screenRedraw)//pause CPU unless screen needs to update
	      {
		P1OUT &= ~GREEN_LED; //turn off LED when cpu is off
		or_sr(0x10);//cpu off
	      }
	    P1OUT |= GREEN_LED;
	    screenRedraw = 0;
	    drawString5x7(2, 4, "Score:", COLOR_PINK, COLOR_BLUE);
	    drawString5x7(40, 4, &score, COLOR_PINK, COLOR_BLUE);
	    paddleMvmt(button);
	    movLayerDraw(&ballMove, &ball);
	    if(score == '9')
	      {
		screenOn = 2;
	      }
	  }
      }
  }


  /** Watchdog timer interrupt handler. 15 interrupts/sec */
  void wdt_c_handler()
  {
    static short cnt = 0;
    P1OUT |= GREEN_LED;
    cnt ++;
    if(cnt == 20)
      {
	if(screenOn == 1)
	  m1Advance(&ballMove, &boundary);
	if(p2sw_read())
	  screenRedraw = 1;
	cnt = 0;
      }
    P1OUT &= ~GREEN_LED;
  }
