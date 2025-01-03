#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

// Pin and configuration constants
#define BUTTON_PIN GPIO_PIN_12          // Button connected to pin PB12
#define NUM_LEDS 5                      // Number of LEDs
const uint16_t ledPins[NUM_LEDS] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_0, GPIO_PIN_1}; // Array of LED pins (PA0, PA1, PA3, PB0, PB1)

#define BUZZER_PIN GPIO_PIN_14          // Buzzer connected to pin PC14
#define LED_DELAY 1000                  // Delay between LED transitions (ms)
#define MIN_WAIT_DELAY 500             // Minimum random wait delay (ms)
#define MAX_WAIT_DELAY 5000            // Maximum random wait delay (ms)

// Variables to track button press and timing
volatile uint8_t btnPressed = 0;      // Set by the ISR when the button is pressed
uint32_t btnPressedTime = 0;
uint32_t reactionTime = 0;
uint32_t ledsOffTime = 0;
uint32_t record = 5000;               // Initial record time in ms

// Interrupt Service Routine for button press
void EXTI15_10_IRQHandler(void) {
  if (__HAL_GPIO_EXTI_GET_IT(BUTTON_PIN) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(BUTTON_PIN); // Clear the interrupt flag
    btnPressed = 1;                       // Set button pressed flag
    btnPressedTime = HAL_GetTick();       // Record the time of the button press
  }
}

// Function to initialize GPIO pins
void GPIO_Init(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE(); // Enable GPIOB clock
  __HAL_RCC_GPIOC_CLK_ENABLE(); // Enable GPIOC clock
  __HAL_RCC_GPIOA_CLK_ENABLE(); // Enable GPIOA clock

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Initialize button pin (PB12)
  GPIO_InitStruct.Pin = BUTTON_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // Interrupt on falling edge
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Initialize LED pins
  for (int i = 0; i < NUM_LEDS; i++) {
    GPIO_InitStruct.Pin = ledPins[i];
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output mode
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, ledPins[i], GPIO_PIN_RESET);  // Ensure LEDs are off
  }

  // Initialize buzzer pin (PC14)
  GPIO_InitStruct.Pin = BUZZER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_RESET);  // Ensure buzzer is off
}

// Function to initialize the Timer for delays (if needed for buzzer or timing)
void Timer_Init(void) {
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);  // Configure SysTick to generate 1ms ticks
}

// Function to play melody
void playMelody(uint16_t *melody, uint16_t *durations, uint8_t length) {
  for (int i = 0; i < length; i++) {
    HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_SET);  // Turn on buzzer
    HAL_Delay(durations[i]);                             // Wait for the duration
    HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_RESET); // Turn off buzzer
    HAL_Delay(50); // Short pause between notes
  }
}

// Main game loop
int main(void) {
  HAL_Init();
  GPIO_Init();
  Timer_Init();

  // Configure external interrupt for the button (PB12)
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);  // Enable EXTI interrupt line for PB12

  while (1) {
    // Wait for button press to start the game
    if (!btnPressed) {
      // Display waiting message here (use USART or another method to show it on a screen)
      continue;  // Stay in this loop until button is pressed
    }

    // Reset button press flag
    btnPressed = 0;

    // Countdown before the game starts (Display this on an LCD or USART)
    HAL_Delay(3000);  // Simulated countdown

    // Sequentially turn on LEDs and beep
    for (int i = 0; i < NUM_LEDS; i++) {
      if (btnPressed) {
        // Button pressed too early, play losing melody
        uint16_t loseMelody[] = {220, 196, 174, 164, 147};
        uint16_t loseDurations[] = {300, 300, 300, 300, 300};
        playMelody(loseMelody, loseDurations, 5);

        // Turn off all LEDs and stop the game
        for (int k = 0; k < NUM_LEDS; k++) {
          HAL_GPIO_WritePin(GPIOA, ledPins[k], GPIO_PIN_RESET);
        }
        HAL_Delay(3000);
        continue;  // Restart the game loop
      }

      HAL_GPIO_WritePin(GPIOA, ledPins[i], GPIO_PIN_SET);  // Turn on LED
      // Play beep sound (this can be done using a timer interrupt or delay)
      HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_SET);
      HAL_Delay(200);  // Beep duration
      HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_RESET);
      HAL_Delay(LED_DELAY - 200);  // Remaining time with silence
    }

    // Random delay before turning LEDs off
    uint32_t waitDelay = MIN_WAIT_DELAY + (rand() % (MAX_WAIT_DELAY - MIN_WAIT_DELAY + 1));
    HAL_Delay(waitDelay);

    // Turn off LEDs and measure reaction time
    ledsOffTime = HAL_GetTick();
    for (int i = 0; i < NUM_LEDS; i++) {
      HAL_GPIO_WritePin(GPIOA, ledPins[i], GPIO_PIN_RESET);
    }

    // Start buzzer sound without blocking
    HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_SET);  // Higher frequency
    btnPressed = 0;

    while (!btnPressed) {
      // Wait for ISR to set btnPressed
    }

    // Stop buzzer sound
    HAL_GPIO_WritePin(GPIOC, BUZZER_PIN, GPIO_PIN_RESET);

    // Calculate reaction time
    reactionTime = HAL_GetTick() - ledsOffTime;

    // Check for new record and play winning sound
    if (reactionTime < record) {
      record = reactionTime;
      // Play winning melody
      uint16_t winMelody[] = {262, 294, 330, 349, 392, 440};
      uint16_t winDurations[] = {300, 300, 300, 300, 300, 300};
      playMelody(winMelody, winDurations, 6);
    }

    HAL_Delay(5000);  // Wait before restarting the game
  }
}



