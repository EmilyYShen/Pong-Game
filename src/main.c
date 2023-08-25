/*
 * ESE3500_Lab4_Pong.c
 *
 * Author : E.Shen
 */

#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)

#include "../lib/ST7735.h"
#include "../lib/LCD_GFX.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <util/delay.h>

/*----------------------UART------------------------------*/
void uart_init() {
    // Set baud rate
    UBRR0H = (uint8_t) (BAUD_PRESCALER >> 8);
    UBRR0L = (uint8_t) (BAUD_PRESCALER);

    // Enable receiver and transmitter
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

    // Set frame format: 8 data bits, 1 stop bit
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);
}

void uart_putchar(char c) {
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)));

    // Put data into buffer, sends the data
    UDR0 = c;
}

char uart_printchar(char* StringPointer) {
    while(*StringPointer != 0x00)
    {
        uart_putchar(*StringPointer);
        StringPointer++;
    }
}

/******************************************************************************
* Global Variables
******************************************************************************/
// used for uart
char String[25];

// computer paddle
int cPaddlePos = 63;
bool cPaddleDirection = false;

// user paddle
int cPaddlePosUser = 63;

// ball
int pX = 80; // x position
int pY = 63; // y position
int vX; // x velocity
int vY; // y velocity

// scoreboard (out of 5)
int userScore = 0;
int computerScore = 0;

// round scoreboard (out of 3)
int userRoundScore = 0;
int computerRoundScore = 0;

/******************************************************************************
* Initialize
******************************************************************************/
void Initialize() {
    cli();

    // set screen color to white
    LCD_setScreen(rgb565(255, 255, 255));

    // initialize user paddle
    LCD_drawBlock(3, 48, 8, 78, 0);

    // initialize computer paddle
    LCD_drawBlock(151, 48, 156, 78, 0);

    // initialize ball
    LCD_drawCircle(80, 63, 3, 0);

    // draw scoreboard
    LCD_drawBlock(0, 0, 160, 10, 0);
    LCD_drawLine(70, 0, 70, 10, rgb565(255,255,255));
    LCD_drawLine(90, 0, 90, 10, rgb565(255,255,255));
    LCD_drawLine(80, 0, 80, 10, rgb565(255,255,255));
    LCD_drawLine(70, 10, 90, 10, rgb565(255,255,255));

    // initialize scores
    LCD_drawChar(73, 2, userRoundScore + 48, rgb565(255,255,255), 0);
    LCD_drawChar(83, 2, computerRoundScore + 48, rgb565(255,255,255), 0);

    // initialize round scores
    sprintf(String, "Wins: %d", userRoundScore);
    LCD_drawString(5, 2, String, rgb565(255,255,255), 0);
    sprintf(String, "Wins: %d", computerRoundScore);
    LCD_drawString(110, 2, String, rgb565(255,255,255), 0);

    // Set PD4 as output for red LED (computer)
    DDRD |= (1 << DDD4);

    // Set PD3 as output for green LED (user)
    DDRD |= (1 << DDD3);

    // Set PD2 as output for buzzer
    DDRD |= (1 << DDD2);

    // setup for ADC
    // input PA0 pin
    DDRC &= ~(1 << DDC0);

    // clear power reduction for ADC
    PRR &= ~(1<<PRADC);

    // select Vref = AVcc
    ADMUX |= (1<<REFS0);
    ADMUX &= ~(1<<REFS1);

    // set the ADC clock div by 128
    ADCSRA |= (1<<ADPS0);
    ADCSRA |= (1<<ADPS1);
    ADCSRA |= (1<<ADPS2);

    // select channel 0
    ADMUX &= ~(1<<MUX0);
    ADMUX &= ~(1<<MUX1);
    ADMUX &= ~(1<<MUX2);
    ADMUX &= ~(1<<MUX3);

    // set to free running
    ADCSRB &= ~(1<<ADTS0);
    ADCSRB &= ~(1<<ADTS1);
    ADCSRB &= ~(1<<ADTS2);

    // disable digital input buffer on ADC pin
    DIDR0 |= (1<<ADC0D);

    // enable ADC
    ADCSRA |= (1<<ADEN);

    // start conversion
    ADCSRA |= (1<<ADSC);

    // generate random speed and direction
    srand(time(NULL));

    // generate random x direction
    int direction = rand() % 8;
    if (direction <= 4) {  // Randomize x-direction
        vX = (rand() % 4) * -1 - 1;
    } else {
        vX = (rand() % 4) + 1;
    }

    // generate random y direction
    direction = rand() % 8;
    if (direction <= 4) {  // Randomize y-direction
        vY = (rand() % 4) * -1 - 1;
    } else {
        vY = (rand() % 4) + 1;
    }

    sei();
}

// update user paddle based on ADC value
void updateUserPaddle() {
    if (ADC <= 490) {
        // check if paddle has hit bounds
        if (cPaddlePosUser - 15 >= 15) {
            // move paddle up
            cPaddlePosUser -= 5;
        }
    } else if (ADC >= 550) {
        // check if paddle has hit bounds
        if (cPaddlePosUser + 15 <= 122) {
            // move paddle down
            cPaddlePosUser += 5;
        }
    }

    // redraw paddle
    LCD_drawBlock(3, 11, 8, 127, rgb565(255,255,255));
    LCD_drawBlock(3, cPaddlePosUser - 15, 8, cPaddlePosUser + 15, 0);

    // restart conversion
    ADCSRA |= (1<<ADSC);
}

// set up computer paddle
void setComputerPaddle() {
    // check if paddle has hit bounds
    if ((cPaddlePos + 5) > 110) {
        cPaddleDirection = true;
    }
    if ((cPaddlePos - 5) < 27) {
        cPaddleDirection = false;
    }

    if (cPaddleDirection) {
        // move paddle up
        cPaddlePos = cPaddlePos - 5;
    } else {
        // move paddle down
        cPaddlePos = cPaddlePos + 5;
    }

    // replace previous computer paddle
    LCD_drawBlock(151, 11, 156, 127, rgb565(255,255,255));
    LCD_drawBlock(151, cPaddlePos - 15, 156, cPaddlePos + 15,0);

}

// update the position of the ball
void updateBallPosition() {
    // update ball position with the velocity
    pX += vX;
    pY += vY;

    // replace previous ball
    LCD_drawCircle(pX - vX, pY - vY, 3,rgb565(255, 255, 255));
    LCD_drawCircle(pX, pY, 3,0);
}

// check if this round has finished
void checkRoundFinish() {
    // check if user has won 5 games in this round
    if (userScore == 5) {
        // reset scores
        userScore = 0;
        computerScore = 0;

        // replace scores
        LCD_drawChar(73, 2, '0', rgb565(255,255,255), 0);
        LCD_drawChar(83, 2, '0', rgb565(255,255,255), 0);

        // update round scores
        userRoundScore += 1;
        sprintf(String, "Wins: %d", userRoundScore);
        LCD_drawString(5, 2, String, rgb565(255,255,255), 0);

        // update to next round
        if (userRoundScore + computerRoundScore < 3) {
            // indicate round number
            sprintf(String, "Round %d", (userRoundScore + computerRoundScore + 1));
            LCD_drawString(50, 63, String, 0, rgb565(255,255,255));

            _delay_ms(1000);

            LCD_drawString(50, 63, String, rgb565(255,255,255),
                           rgb565(255,255,255));
        }
    }
    // check if computer has won 5 games in this round
    if (computerScore == 5) {
        // reset scores
        userScore = 0;
        computerScore = 0;

        // replace scores
        LCD_drawChar(73, 2, '0', rgb565(255,255,255), 0);
        LCD_drawChar(83, 2, '0', rgb565(255,255,255), 0);

        // update round scores
        computerRoundScore += 1;
        sprintf(String, "Wins: %d", computerRoundScore);
        LCD_drawString(110, 2, String, rgb565(255,255,255), 0);

        // update to next round
        if (userRoundScore + computerRoundScore < 3) {
            // indicate round number
            sprintf(String, "Round %d", (userRoundScore + computerRoundScore + 1));
            LCD_drawString(50, 63, String, 0, rgb565(255,255,255));

            _delay_ms(1000);

            LCD_drawString(50, 63, String, rgb565(255,255,255),
                           rgb565(255,255,255));
        }
    }

    // turn off LEDs
    PORTD &= ~(1 << PORTD3);
    PORTD &= ~(1 << PORTD4);

    // check if there have been three rounds
    if ((userRoundScore + computerRoundScore) == 3) {

        // check and display winner
        LCD_setScreen(0);
        if (userRoundScore > computerRoundScore) {
            // light up green LED if user wins
            PORTD |= (1 << PORTD3);
            LCD_drawString(50, 63, "user wins!", rgb565(255,255,255),
                         0);
        } else {
            // light up red LED if computer wins
            PORTD |= (1 << PORTD4);
            LCD_drawString(40, 63, "computer wins!", rgb565(255,255,255),
                         0);
        }

        _delay_ms(5000);

        // restart game
        lcd_init();
        LCD_setScreen(0);
        LCD_drawString(50, 63, "Round 1", rgb565(255,255,255),
                       0);

        _delay_ms(1000);

        userScore = 0;
        computerScore = 0;
        userRoundScore = 0;
        computerRoundScore = 0;

        Initialize();
    }
}

// update scoreboard for user
void updateScoreboardUser() {
    // light up green LED
    PORTD |= (1 << PORTD3);

    // update userScore variable
    userScore = userScore + 1;

    // replace user score
    LCD_drawChar(73, 2, userScore + 48, rgb565(255,255,255), 0);

    // check if round has finished
    checkRoundFinish();

    // reset the game
    // initialize ball
    LCD_drawCircle(pX, pY, 3, rgb565(255,255,255));
    LCD_drawCircle(80, 63, 3, 0);

    // initialize user paddle
    LCD_drawBlock(3, cPaddlePosUser - 15, 8, cPaddlePosUser + 15,
                  rgb565(255, 255, 255));
    LCD_drawBlock(3, 48, 8, 78, 0);

    // initialize computer paddle
    LCD_drawBlock(151, cPaddlePos - 15, 156, cPaddlePos + 15,
                  rgb565(255, 255, 255));
    LCD_drawBlock(151, 48, 156, 78, 0);

    // reset paddles
    cPaddlePos = 63;
    cPaddlePosUser = 63;

    // generate random speed and direction for next game (with min of -5 and max of 5)
    pX = 80;
    pY = 63;
    int direction = rand() % 10;
    if (direction <= 5) {  // Randomize x direction
        vX = (rand() % 5) * -1 - 1;
    } else {
        vX = (rand() % 5) + 1;
    }
    direction = rand() % 10;
    if (direction <= 5) {  // Randomize y direction
        vY = (rand() % 5) * -1 - 1;
    } else {
        vY = (rand() % 5) + 1;
    }

    _delay_ms(1000);

    // turn off green LED
    PORTD &= ~(1 << PORTD3);
}

// update scoreboard for computer
void updateScoreboardComputer() {
    // light up red LED
    PORTD |= (1 << PORTD4);

    // update computerScore variable
    computerScore = computerScore + 1;

    // replace computer score
    LCD_drawChar(83, 2, computerScore + 48, rgb565(255,255,255), 0);

    // check if round has finished
    checkRoundFinish();

    // reset the game
    // initialize ball
    LCD_drawCircle(pX, pY, 3, rgb565(255,255,255));
    LCD_drawCircle(80, 63, 3, 0);

    // initialize user paddle
    LCD_drawBlock(3, cPaddlePosUser - 15, 8, cPaddlePosUser + 15,
                  rgb565(255, 255, 255));
    LCD_drawBlock(3, 48, 8, 78, 0);

    // initialize computer paddle
    LCD_drawBlock(151, cPaddlePos - 15, 156, cPaddlePos + 15,
                  rgb565(255, 255, 255));
    LCD_drawBlock(151, 48, 156, 78, 0);

    // reset paddles
    cPaddlePos = 63;
    cPaddlePosUser = 63;

    // generate random speed and direction (with min of -5 and max of 5)
    pX = 80;
    pY = 63;
    int direction = rand() % 10;
    if (direction <= 5) {  // Randomize x direction
        vX = (rand() % 5) * -1 - 1;
    } else {
        vX = (rand() % 5) + 1;
    }
    direction = rand() % 10;
    if (direction <= 5) {  // Randomize y direction
        vY = (rand() % 5) * -1 - 1;
    } else {
        vY = (rand() % 5) + 1;
    }

    _delay_ms(1000);

    // turn off red LED
    PORTD &= ~(1 << PORTD4);
}

// check if ball is out of bounds
void checkOutOfBounds() {
    // check user paddle edge
    if ((vX <= 0) && (abs(vX) >= pX - 8)) {
        // toggle buzzer
        PORTD ^= (1 << PORTD2);
        _delay_ms(20);
        PORTD ^= (1 << PORTD2);

        // change velocity
        vX = - vX;

        // check if user misses
        if ((pY < (cPaddlePosUser - 16)) || (pY > (cPaddlePosUser + 16))) {
            updateScoreboardComputer();
        }
    }
    // check top of screen
    if ((vY <= 0) && (abs(vY) >= pY - 13)) {
        // toggle buzzer
        PORTD ^= (1 << PORTD2);
        _delay_ms(20);
        PORTD ^= (1 << PORTD2);

        // change velocity
        vY = - vY;

    }
    // check computer paddle edge
    if ((vX >= 0) && (pX >= (150 - vX))) {
        // toggle buzzer
        PORTD ^= (1 << PORTD2);
        _delay_ms(20);
        PORTD ^= (1 << PORTD2);

        // change velocity
        vX = - vX;

        // check if computer misses
        if ((pY < (cPaddlePos - 16)) || (pY > (cPaddlePos + 16))) {
            updateScoreboardUser();
        }
    }
    // check bottom of screen
    if ((vY >= 0) && (pY >= (125 - vY))) {
        // toggle buzzer
        PORTD ^= (1 << PORTD2);
        _delay_ms(20);
        PORTD ^= (1 << PORTD2);

        // change velocity
        vY = - vY;
    }
}

int main(void) {

    // begin game
    lcd_init();
    LCD_setScreen(0);
    LCD_drawString(50, 63, "Round 1", rgb565(255,255,255),
                   0);

    _delay_ms(1000);

    // initialize game
    Initialize();

    uart_init();

    while(1) {
        updateUserPaddle();
        setComputerPaddle();
        updateBallPosition();
        checkOutOfBounds();
    }
}