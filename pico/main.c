
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "bme280.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt_priv.h"
#include <malloc.h>

const char *MQTT_BROKER_URL = "192.168.1.100";

static ip_addr_t mqtt_ip;

typedef struct MQTT_CLIENT_DATA_T {
        mqtt_client_t *mqtt_client_inst;
        struct mqtt_connect_client_info_t mqtt_client_info;
        uint8_t data[MQTT_OUTPUT_RINGBUF_SIZE];
        uint8_t topic[100];
        uint32_t len;
        bool playing;
        bool newTopic;
} MQTT_CLIENT_DATA_T;

MQTT_CLIENT_DATA_T *mqtt;

struct mqtt_connect_client_info_t mqtt_client_info = {
        "test",
        "jemi", 
        NULL,      
        15,      
        NULL,     
        NULL,     
        0,         
        0          
#if LWIP_ALTCP && LWIP_ALTCP_TLS
        ,
        NULL
#endif
};

//void print_memory_usage() {
    //struct mallinfo info = mallinfo();

    //printf("Total allocated: %d bytes\n", info.uordblks);
    //printf("Total free: %d bytes\n", info.fordblks);
    //printf("Total heap size: %d bytes\n", info.arena);
    //printf("Largest free block: %d bytes\n", info.ordblks);
//}

/* Called when publish is complete either with sucess or failure */
void mqtt_pub_request_cb(void *arg, err_t result)
{
        if (result != ERR_OK){
        printf("Publish result not OK: %d\n", result);
        }
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
        printf("mqtt_incoming_publish_cb\n");
        MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;
        // strcpy(mqtt_client->topic, topic);
        strcpy((char *)mqtt_client->topic, (char *)topic);
}

static void mqtt_request_cb(void *arg, err_t err)
{
        MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;
        LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", mqtt_client->mqtt_client_info.client_id, (int)err));
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
        MQTT_CLIENT_DATA_T *mqtt_client = (MQTT_CLIENT_DATA_T *)arg;
        LWIP_UNUSED_ARG(client);

        LWIP_PLATFORM_DIAG(("MQTT client \"%s\" connection cb: status %d\n", mqtt_client->mqtt_client_info.client_id, (int)status));

        if (status == MQTT_CONNECT_ACCEPTED) {
                printf("MQTT_CONNECT_ACCEPTED\n");
        }
}

void mqtt_connect(MQTT_CLIENT_DATA_T *mqtt_client, void *arg)
{
        err_t err;
        cyw43_arch_lwip_begin();
        err = mqtt_client_connect(mqtt_client->mqtt_client_inst, &mqtt_ip, MQTT_PORT, mqtt_connection_cb, arg, &mqtt_client->mqtt_client_info);
        cyw43_arch_lwip_end();
        if (err != ERR_OK){
                //printf("mqtt_connect return %d\n", err);
        }
}

int main() {
	
	stdio_init_all();

	if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
                 printf("Wi-Fi init failed");
                return -1;
        }

	cyw43_arch_enable_sta_mode();

	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)){
                printf("failed to connect.\n");
                return 1;
        }
	else {
                printf("Connected.\n");
        }

	// Initialize mqtt client
        mqtt = (MQTT_CLIENT_DATA_T *)calloc(1, sizeof(MQTT_CLIENT_DATA_T));
        if (!mqtt){
                printf("Failed to allocate memory for mqtt client\n");
                return -1;
        }

	mqtt->playing = false;
        mqtt->newTopic = false;
        mqtt->mqtt_client_info = mqtt_client_info;

        mqtt->mqtt_client_inst = mqtt_client_new();


	if (!ip4addr_aton(MQTT_BROKER_URL, &mqtt_ip)){
                printf("ip error\n");
                return 0;
        }
	
	 //Connect to mqtt broker
        err_t err = mqtt_client_connect(mqtt->mqtt_client_inst,
                    &mqtt_ip, MQTT_PORT,
                    mqtt_connection_cb, LWIP_CONST_CAST(void *, &mqtt->mqtt_client_info),
                    &mqtt->mqtt_client_info);
        if (err != ERR_OK){
                printf("mqtt_connect return %d\n", err);
                return -1;
        }
	
    	// This example will use SPI0 at 0.5MHz.
    	spi_init(spi_default, 500 * 1000);
    	gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    	gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    	// Make the SPI pins available to picotool
    	bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    	// Chip select is active-low, so we'll initialise it to a driven-high state
    	gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    	gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    	gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    	// Make the CS pin available to picotool
    	bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    	// See if SPI is working - interrograte the device for its I2C ID number, should be 0x60
    	uint8_t id;
    	read_registers(0xD0, &id, 1);
    	printf("Chip ID is 0x%x\n", id);

    	read_compensation_parameters();

    	write_register(0xF2, 0x1); 
    	write_register(0xF4, 0x27);

    	int32_t humidity, pressure, temperature;
	
	mqtt_connect(mqtt, mqtt);
	free(mqtt);
    	while (1) {
       		
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
		sleep_ms(500);
		bme280_read_raw(&humidity, &pressure, &temperature);


        	temperature = compensate_temp(temperature);
        	pressure = compensate_pressure(pressure);
        	humidity = compensate_humidity(humidity);

        	printf("Humidity = %.2f%%\n", humidity / 1024.0);
        	printf("Pressure = %dPa\n", pressure);
        	printf("Temp. = %.2fC\n", temperature / 100.0);
		char buf[100];
		sprintf(buf, "%.2f %.3f %.2f", temperature / 100.f, pressure / 1000.f, (double)humidity / (double)1024.0);
		char topic[100];
		sprintf(topic, "pico_bme280");
		mqtt_publish(mqtt->mqtt_client_inst, "pico_bme280", buf, strlen(buf), 0, 0, mqtt_pub_request_cb, mqtt);
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
		//print_memory_usage();

        	sleep_ms(1000);
    	}
}
