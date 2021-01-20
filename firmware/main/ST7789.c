#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "ST7789.h"
#include "ST7789_commands.h"

#include "pins.h"
#include "peripherals.h"  // Defines the SPI host and DMA channel

// Global SPI device handle
static spi_device_handle_t spi;

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

#define DELAY_FLAG 0x80
#define END_OF_CMDS 0xFF

//Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
DRAM_ATTR static const lcd_init_cmd_t lcd_init_cmds[]={
    /* Memory Data Access Control, MY=MV=1, MX=ML=MH=0, RGB=0 */
    {MADCTL, {MADCTL_MY | MADCTL_MV}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {COLMOD, {COLMOD_16BIT}, 1},
    /* Porch Setting */
    {PORCTRL, {0x0c, 0x0c, PORCTRL_DISABLE, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {GCTRL, {VGH_13_65V | VGL_10_43V}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {VCOMS, {VCOMS_1_175V}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {LCMCTRL, {LCM_XMX | LCM_XMH}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {VDVVRHEN, {VDVVRH_CMDEN, VDVVRH_DUMMY}, 2},
    /* VRH Set, Vap=4.4+... */
    {VRHS, {VRH_4_4V}, 1},
    /* VDV Set, VDV=0 */
    {VDVS, {VDV_0V}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {FRCTRL2, {FR_60HZ}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {PWCTRL1, {PWCTRL_DUMMY, AVDD_6_8V | AVCL_4_8V | VDDS_2_3V}, 1},
    /* Positive Voltage Gamma Control */
    {PVGAMCTRL, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    /* Negative Voltage Gamma Control */
    {NVGAMCTRL, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
    /* Sleep Out */
    {SLPOUT, {0}, DELAY_FLAG},
    /* Inversion off */
    {INVON, {0}, DELAY_FLAG},
    /* Display On */
    {DISPON, {0}, DELAY_FLAG},
    {0, {0}, END_OF_CMDS}
};

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void lcd_cmd(const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void lcd_data(const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(ST7789_DC, dc);
}

static void init_lcd_spi() 
{
    //Initialize the SPI bus
    spi_bus_config_t buscfg={
        .miso_io_num=ST7789_SPI_MISO,
        .mosi_io_num=ST7789_SPI_MOSI,
        .sclk_io_num=ST7789_SPI_SCK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=MAX_PIXEL_TRANSACTION*2+8
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ST7789_HOST, &buscfg, ST7789_DMA_CHAN));

    //Attach the LCD to the SPI bus
    spi_device_interface_config_t devcfg={
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz=26*1000*1000,           //Clock out at 26 MHz
#else
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
#endif
        .mode=0,                                //SPI mode 0
        .spics_io_num=ST7789_SPI_CS,            //CS pin
        //We want to be able to queue 7 transactions at a time
        .queue_size=7,             
        //Specify pre-transfer callback to handle D/C line             
        .pre_cb=lcd_spi_pre_transfer_callback, 
    };
    ESP_ERROR_CHECK(spi_bus_add_device(ST7789_HOST, &devcfg, &spi));
}

//Initialize the display
static void init_st7789()
{
    //Initialize non-SPI GPIOs
    gpio_set_direction(ST7789_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(ST7789_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(ST7789_BCKL, GPIO_MODE_OUTPUT);

    //Reset the display
    gpio_set_level(ST7789_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(ST7789_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    //Send all the commands
    int cmd=0;
    while (lcd_init_cmds[cmd].databytes != END_OF_CMDS) {
        lcd_cmd(lcd_init_cmds[cmd].cmd);
        lcd_data(lcd_init_cmds[cmd].data, 
                 lcd_init_cmds[cmd].databytes & 0x1F);
        if (lcd_init_cmds[cmd].databytes & DELAY_FLAG) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }

    ///Enable backlight
    gpio_set_level(ST7789_BCKL, 1);
}

static spi_transaction_t pixel_trans[6];
uint16_t* pixel_tx_buffer;

static void init_pixel_trans() {
    pixel_tx_buffer = malloc(MAX_PIXEL_TRANSACTION * sizeof(uint16_t));
    memset(pixel_trans, 0, sizeof(spi_transaction_t) * 6);
    for (int t_idx=0; t_idx<6; t_idx++) {
        if ((t_idx&1)==0) {
            //Even transfers are commands
            pixel_trans[t_idx].length=8;
            pixel_trans[t_idx].user=(void*)0;
        } else {
            //Odd transfers are data
            pixel_trans[t_idx].length=8*4;
            pixel_trans[t_idx].user=(void*)1;
        }
        if (t_idx != 5) {
            // Only use separate txbuffer for the RAMWR command data
            pixel_trans[t_idx].flags=SPI_TRANS_USE_TXDATA;
        }
    }

    //Set commands
    pixel_trans[0].tx_data[0]=CASET;  //Column Address Set
    pixel_trans[2].tx_data[0]=RASET;  //Page address set
    pixel_trans[4].tx_data[0]=RAMWR;  //Memory write
    pixel_trans[5].tx_buffer=pixel_tx_buffer;
}

void initialize_display()
{
    init_lcd_spi();
    init_st7789();
    init_pixel_trans();
}

static bool pixel_trans_active = false;

static void finish_pixel_transaction()
{
    if (!pixel_trans_active) { return; }
    spi_transaction_t *rtrans;
    //Wait for all 6 transactions to be done and get back the results.
    for (int t_idx=0; t_idx<6; t_idx++) {
        ESP_ERROR_CHECK(spi_device_get_trans_result(
            spi, &rtrans, portMAX_DELAY));
    }
    pixel_trans_active = false;
}

static void send_pixels_single(
        uint16_t xpos, uint16_t ypos, 
        uint16_t width, uint16_t height, 
        uint16_t *data)
{
    assert(width * height <= MAX_PIXEL_TRANSACTION);

    // Wait for any previous transactions to clear
    finish_pixel_transaction();

    // Set window dimensions
    uint16_t xend = xpos + width;
    uint16_t yend = ypos + height;
    pixel_trans[1].tx_data[0] = (xpos >> 8);    //Start Col High
    pixel_trans[1].tx_data[1] = (xpos & 0xFF);  //Start Col Low
    pixel_trans[1].tx_data[2] = (xend >> 8);    //End Col High
    pixel_trans[1].tx_data[3] = (xend & 0xFF);  //End Col Low
    pixel_trans[3].tx_data[0] = (ypos >> 8);    //Start page high
    pixel_trans[3].tx_data[1] = (ypos & 0xFF);  //Start page low
    pixel_trans[3].tx_data[2] = (yend >> 8);    //End page high
    pixel_trans[3].tx_data[3] = (yend & 0xFF);  //End page low
    
    // Setup the TX buffer
    size_t pixel_bytes = width * height * sizeof(uint16_t);
    memcpy((void*)pixel_tx_buffer, (void*)data, pixel_bytes);
    pixel_trans[5].length    = pixel_bytes * 8; //bits

    //Queue all transactions.
    for (int t_idx=0; t_idx<6; t_idx++) {
        ESP_ERROR_CHECK(spi_device_queue_trans(spi, 
            &pixel_trans[t_idx], portMAX_DELAY));
    }
    pixel_trans_active = true;
}

static inline int min(int a, int b) { return (a < b) ? a : b; }

void send_pixels(xcoord_t xpos, ycoord_t ypos, 
                 xcoord_t width, ycoord_t height, 
                 color_t *data)
{
    if (width * height <= MAX_PIXEL_TRANSACTION) {
        send_pixels_single(xpos, ypos, width, height, data);
    } else {
        int lines_per_block = MAX_PIXEL_TRANSACTION / width;
        int curr_line = 0;
        while (curr_line < height) {
            int b_h = min(lines_per_block, height - curr_line);
            send_pixels_single(xpos, ypos + curr_line, width, b_h, data);
            curr_line += b_h;
            data += (width * b_h);
        }
    }
}

void blank_screen()
{
    size_t buffer_size = sizeof(uint16_t) * DISPLAY_WIDTH * MAX_LINES;
    uint16_t* buffer = malloc(buffer_size);
    memset(buffer, 0, buffer_size);
    int curr_line = 0;
    while (curr_line < DISPLAY_HEIGHT) {
        int h = min(MAX_LINES, DISPLAY_HEIGHT - curr_line);
        send_pixels_single(0, curr_line, DISPLAY_WIDTH, h, buffer);
        curr_line += h;
    }
}