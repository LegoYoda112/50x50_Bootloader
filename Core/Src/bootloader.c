#include "bootloader.h"

enum bootloaderState bootloader_state;
typedef void (*pFunction)(void);

uint32_t FirstPage = 0, NbOfPages = 0;
uint32_t Address = 0, PageError = 0;
__IO uint32_t MemoryProgramStatus = 0;
__IO uint64_t data64 = 0;

static FLASH_EraseInitTypeDef EraseInitStruct;

static uint32_t GetPage(uint32_t Addr)
{
  return (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;;
}

void process_USB_rx(uint8_t* Buf, uint32_t *Len){

    // Clear 
    if(Buf[0] == 'C' && Buf[1] == 'L' && Buf[2] == 'R'){
        erase_flash();
        Address = 0;
    }

    // 
    if(Buf[0] == 'L' && Buf[1] == 'O' && Buf[2] == 'D'){
        bootloader_jump_to_user_app();
    }

    // Receive ihex data
    if(Buf[0] == ':'){
        char record_substring[3];
        strncpy(record_substring, Buf + (uint32_t)7, 2);
        volatile uint32_t record_type = strtol(record_substring, NULL, 16);

        // Process data record
        if(record_substring[0] == '0' && record_substring[1] == '0') {
            
            // Hardcode data length
            uint16_t data_length = 16;
            uint8_t data[16];
            // Record strings are 16 byte long = 2x 64bit double words
            uint64_t double_word_1 = 0;
            uint64_t double_word_2 = 0;

            // Extract data array from hex string and fill double words
            for(int i = 0; i < data_length; i++){

                // Convert hex byte to uint8
                char data_substring[3];
                strncpy(data_substring, Buf + 9 + 2 * i, 2);
                data_substring[2] = '\0';
                data[i] = strtol(data_substring, NULL, 16);

                // Fill double_word_1
                if(i < 8){
                    double_word_1 = double_word_1 | ((uint64_t)data[i] << i * 8);
                }else{ // Fill double_word_2
                    double_word_2 = double_word_2 | ((uint64_t)data[i] << (i - 8) * 8);
                }
            }

            // Calculate flash address
            uint32_t offset_address = (uint32_t)APP_START_ADDR + Address;

            // Flash two double words
            HAL_FLASH_Unlock();
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, offset_address, double_word_1) != HAL_OK){
                Error_Handler();
            }
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, offset_address + 0x08, double_word_2) != HAL_OK){
                Error_Handler();
            }
            HAL_FLASH_Lock();

            // Increment address
            Address += 16;
        }

    }

    uint8_t *data = "OK\r";
    CDC_Transmit_FS(data, strlen(data));
    __enable_irq();
}


void bootloader_jump_to_user_app(void)
{
    void (*app_reset_handler)(void);

    // DeInit all
    HAL_GPIO_DeInit(LED_R_GPIO_Port, LED_R_Pin);
    HAL_GPIO_DeInit(LED_G_GPIO_Port, LED_G_Pin);
    HAL_GPIO_DeInit(LED_B_GPIO_Port, LED_B_Pin);
    HAL_FDCAN_DeInit(&hfdcan1);
    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Disable interrupts
    __set_PRIMASK(1);
    __disable_irq();

    // SCB->VTOR = FLASH_SECTOR2_BASE_ADDRESS; //0x080080000

    // Set main stack pointer
    uint32_t msp_value = *(__IO uint32_t *)APP_START_ADDR;
    __set_MSP(msp_value);

    // Set up reset handler function pointer
    uint32_t resethandler_address = *(__IO uint32_t *) (APP_START_ADDR + 4);
    app_reset_handler = (void*) resethandler_address;

    // Call app reset handler to jump to app code
    app_reset_handler();

}

void bootloader_init(){

}

void bootloader_loop(){

    // Blink Twice
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
    HAL_Delay(100);
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
    HAL_Delay(100);
}

void erase_flash(){
    
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR); // Not quite sure this was included in example code

    // Set up erase pages
    FirstPage = GetPage(APP_START_ADDR);
    NbOfPages = GetPage(APP_END_ADDR) - FirstPage + 1;
    // Set up erase struct
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page = FirstPage;
    EraseInitStruct.NbPages = NbOfPages;

    // Perform erase
    if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){
        Error_Handler();
    }

    HAL_FLASH_Lock();
}