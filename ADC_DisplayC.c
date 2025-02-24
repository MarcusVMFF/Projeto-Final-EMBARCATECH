#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pico/bootrom.h"  
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ADC_DisplayC.pio.h"
#include "hardware/timer.h"

//Definições do display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
//Definições da matriz 5x5
#define NUM_PIXELS 25
#define MATRIXPIO 7
//Definições buzzer e microfone
#define BUZZER_PIN 21
#define MIC 28
//Definições PWM
#define PWM_FREQ 1000        
#define CLOCK_DIV 125        
#define WRAP_VALUE 999 
// Definições do joystick, leds e botões
#define VRX_PIN 26  
#define VRY_PIN 27  
#define SW_PIN 22
#define BUTTON_A 5
#define BUTTON_B 6
#define LED_RED 13
#define LED_GREEN 11
// Limite do pulo
#define LIMITE_SUPERIOR 10 
#define LIMITE_INFERIOR 45
//Variaveis globais
int dino_y = 45;
bool pulando = false;
int velocidade_y = 0;
int obstaculo_x = 120;  
int OBSTACULO_Y = 55;
bool jogo_rodando = true;
const uint limiar = 2000;
const uint amostras_por_segundo = 8000;
bool microfone_habilitado = false;

void tocar_buzzer(int freq, int duration_ms)
    {
        uint32_t delay_us = 1000000 / (2 * freq);          
        uint32_t ciclos = (duration_ms * 1000) / delay_us; 

        for (uint32_t t = 0; t < ciclos; t++)
        {
            gpio_put(BUZZER_PIN, 1); 
            sleep_us(delay_us);
            gpio_put(BUZZER_PIN, 0); 
            sleep_us(delay_us);
        }
    }
    
bool som_tocado = false;

double desenhovazio[25] = {0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0};

double desenho4[25] =   {0.3, 0.3, 0.0, 0.3, 0.3,
                        0.3, 0.3, 0.0, 0.3, 0.3, 
                        0.0, 0.0, 0.0, 0.0, 0.0,
                        0.3, 0.3, 0.3, 0.3, 0.3,
                        0.3, 0.0, 0.0, 0.0, 0.3};

void imprimir_binario(int num) {
 int i;
 for (i = 31; i >= 0; i--) {
  (num & (1 << i)) ? printf("1") : printf("0");
 }
}

uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255 * 0.125;
  G = g * 255 * 0.8;
  B = b * 255 * 0.8;
  return (G << 24) | (R << 16) | (B << 8);
}

void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){

    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        if (i%2==0)
        {
            valor_led = matrix_rgb(b=0, desenho[24-i], g=0.0);
            pio_sm_put_blocking(pio, sm, valor_led);

        }else{
            valor_led = matrix_rgb(b=0, desenho[24-i], g=0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    }
    imprimir_binario(valor_led);
}

// Sprite da galinha
uint8_t dino_sprite[32] = {
    0x20, 0x0f, 0xd8, 0x10, 0x04, 0x20, 0x06, 0x20,
    0x75, 0xc0, 0x05, 0x40, 0x05, 0xc0, 0x3b, 0x48,
    0x40, 0x48, 0x40, 0xe8, 0x40, 0xca, 0x20, 0x26,
    0x20, 0x20, 0x10, 0x10, 0x10, 0x09, 0xe0, 0x06
};

uint8_t obstaculo_sprite[8] = {
    0x38, 0x18, 0x7f, 0xff, 0xff, 0x7f, 0x0c, 0x1c
};
uint8_t raposa[32] = {
    0xf0, 0x00, 0x0f, 0x01, 0x01, 0x71, 0x72, 0x8e,
    0x04, 0x80, 0x04, 0x9c, 0x84, 0xfe, 0x04, 0x9c,
    0x04, 0x80, 0x72, 0x8e, 0x01, 0xf1, 0x0f, 0xa1, 
    0xf0, 0x98, 0x00, 0x44, 0x00, 0x38, 0x00, 0x00
};
uint8_t raposagameover[32] = {
    0x00, 0x20, 0x00, 0x58, 0xf0, 0x47, 0x08, 0x81, 
    0x90, 0x8c, 0xa0, 0x80, 0x40, 0x88, 0x40, 0x98, 
    0x40, 0x88, 0xa0, 0x80, 0x90, 0x8c, 0x08, 0x81, 
    0xf0, 0x47, 0x00, 0x58, 0x00, 0x20, 0x00, 0x00
    };
void reiniciar_jogo() {
    dino_y = 45;
    pulando = false;
    velocidade_y = 0;
    obstaculo_x = 120; // Reinicia o obstáculo na posição inicial
    som_tocado = false;
}

bool verificar_colisao() {
    // Checagem simples de colisão baseada em caixas delimitadoras (bounding box)
    if (10 + 16 > obstaculo_x && 10 < obstaculo_x + 8 &&  // Checa sobreposição no eixo X
        dino_y + 16 > OBSTACULO_Y && dino_y < OBSTACULO_Y + 8) { // Checa sobreposição no eixo Y
        return true;
    }
    return false;
}

void draw_dino_sprite(ssd1306_t *ssd, uint8_t x, uint8_t y) {
    // Itera pelas 16 linhas do sprite (agora 16x16)
    for (uint8_t i = 0; i < 16; i++) {
        // Itera pelas 2 bytes (cada byte representa 8 bits do sprite)
        for (uint8_t j = 0; j < 8; j++) {
            // Verifica cada bit de cada byte do sprite e coloca no display
            if (dino_sprite[i * 2] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j, true); // Desenha o primeiro byte
            }
            if (dino_sprite[i * 2 + 1] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j + 8, true); // Desenha o segundo byte
            }
        }
    }
}
void draw_obstaculo(ssd1306_t *ssd, uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < 8; i++) {  
        for (uint8_t j = 0; j < 8; j++) {  
            if (obstaculo_sprite[i] & (1 << j)) {  // Troca linha e coluna para rotacionar
                ssd1306_pixel(ssd, x + i, y + (7 - j), true);
            }
        }
    }
}

void draw_raposa(ssd1306_t *ssd, uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            if (raposa[i * 2] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j, true);
            }
            if (raposa[i * 2 + 1] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j + 8, true);
            }
        }
    }
}
void draw_raposaGameOver(ssd1306_t *ssd, uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            if (raposagameover[i * 2] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j, true);
            }
            if (raposagameover[i * 2 + 1] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j + 8, true);
            }
        }
    }
}
void draw_sprite(ssd1306_t *ssd, uint8_t x, uint8_t y, const uint8_t *sprite) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            if (sprite[i * 2] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j, true);
            }
            if (sprite[i * 2 + 1] & (1 << j)) {
                ssd1306_pixel(ssd, x + (15 - i), y + j + 8, true);
            }
        }
    }
}
void animacao_inicial(ssd1306_t *ssd){
    int dino_x = 70;
    int raposa_x = 10;
    int dino_y = 45;
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
    
    draw_sprite(ssd, dino_x, 45, dino_sprite);
    draw_sprite(ssd, raposa_x, 45, raposa);
    ssd1306_send_data(ssd);
    sleep_ms(2000);
    for (int i = 0; i < 7; i++) {
        ssd1306_fill(ssd, false);
        draw_sprite(ssd, dino_x, dino_y - i, dino_sprite);
        draw_sprite(ssd, raposa_x, 45, raposa); 
        ssd1306_send_data(ssd);
        sleep_ms(20);
    }
    dino_y = 45 - 6;
    sleep_ms(100);
    for (int i = 0; i < 7; i++) {
        ssd1306_fill(ssd, false);
        draw_sprite(ssd, dino_x, dino_y + i, dino_sprite);
        draw_sprite(ssd, raposa_x, 45, raposa); 
        ssd1306_send_data(ssd);
        sleep_ms(20);
    }
    sleep_ms(1000);
    for (int i = 0; i < 60; i++) {
        ssd1306_fill(ssd, false);
        draw_sprite(ssd, dino_x - i, 45, dino_sprite); // Dino indo para x = 10
        draw_sprite(ssd, raposa_x - i, 45, raposa); // Raposa saindo da tela
        ssd1306_send_data(ssd);
        sleep_ms(10);
    }
    sleep_ms(1000);
}
void setup_pwm(uint slice_num, uint gpio_pin) {
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM); 
    pwm_set_clkdiv(slice_num, CLOCK_DIV);        
    pwm_set_wrap(slice_num, WRAP_VALUE);         
    pwm_set_enabled(slice_num, true);            
}
void set_led_intensity(uint slice_num, uint32_t intensity) {
    uint32_t level = (intensity * WRAP_VALUE) / 255; // Mapeia a intensidade (0-255) para o duty cycle
    pwm_set_chan_level(slice_num, PWM_CHAN_B, level); // Define o duty cycle do canal A
}

// Variaveis necessárias para deboucing e funcionamento do botão A
uint32_t last_time_sw = 0;  
uint32_t last_time_btn = 0; 

void alternar_microfone() {
    microfone_habilitado = !microfone_habilitado;  // Inverte o estado do microfone
}

// Rotina de interrupção para o pushbutton do joystick, do botão A e botão B com debouncing
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == SW_PIN && (current_time - last_time_sw > 300000)) {
        last_time_sw = current_time;
        reiniciar_jogo(); 
        jogo_rodando = true;

    }
    if (gpio == BUTTON_A && (current_time - last_time_btn > 300000)) {
        last_time_btn = current_time; 
        if (!pulando) {
            pulando = true;
            velocidade_y = -6;  // Inicia o pulo para cima
        } 
    }
    if (gpio == BUTTON_B) {
        last_time_btn = current_time;
        alternar_microfone();
    }
}

int main() {
    stdio_init_all();

    

    //Inicializa ADC
    adc_init();
    adc_gpio_init(MIC);
    uint64_t intervalo_us = 1000000 / amostras_por_segundo;

    //Inicializações e variavies PWM
    PIO pio = pio0;
    bool ok;
    ok = set_sys_clock_khz(128000, false);
    uint32_t valor_led;
    uint offset = pio_add_program(pio, &ADC_DisplayC_program);
    uint sm = pio_claim_unused_sm(pio, true);
    double r = 0.0, b = 0.0 , g = 0.0;
    uint led_slice_num = pwm_gpio_to_slice_num(LED_GREEN);
    setup_pwm(led_slice_num, LED_GREEN);

    //Inicialização matriz 5x5
    ADC_DisplayC_program_init(pio, sm, offset, MATRIXPIO);

    // Inicializações padrões do display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); 
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); 
    gpio_pull_up(I2C_SDA); 
    gpio_pull_up(I2C_SCL); 
    ssd1306_t ssd; 
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); 
    ssd1306_config(&ssd); 
    ssd1306_send_data(&ssd); 
    
    //Inicializa o LED GPIO
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    // Inicializa todos os pushbuttons
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B); 
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    //Inicializa o buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Configura as interrupções para todos os botões
    gpio_set_irq_enabled(SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);  
    gpio_set_irq_callback(&gpio_irq_handler);
    irq_set_enabled(IO_IRQ_BANK0, true);

    bool cor = true;
    bool som_detectado = false;

    animacao_inicial(&ssd);

    while (true) {
        // Configurações de leitura do ADC
        adc_select_input(2);          
        uint16_t mic_value = adc_read();

        desenho_pio(desenhovazio, valor_led, pio, sm, r, g, b);
        uint32_t intensity = 50;
        ssd1306_fill(&ssd, !cor);

        if (jogo_rodando) {

            gpio_put(LED_RED, false);  // Desliga o LED vermelho
            set_led_intensity(led_slice_num, intensity);
            
            if (microfone_habilitado && mic_value < limiar && !som_detectado) {
                pulando = true;
                velocidade_y = -6;  // Inicia o pulo para cima
                som_detectado = true;  // Marca que o som foi detectado
            }
    
            // Reseta a detecção de som quando o valor do microfone volta ao normal
            if (mic_value >= limiar) {
                som_detectado = false;
            }
            if (pulando) {
                dino_y += velocidade_y;  // Move o dinossauro verticalmente
                velocidade_y += 1;       // Simula a gravidade
                if (dino_y < LIMITE_SUPERIOR) {
                    dino_y = LIMITE_SUPERIOR;  // Impede que o personagem suba além do limite superior
                    velocidade_y = 0;          // Para o movimento para cima
                }
                if (dino_y >= 45) {      // Chegou ao chão
                    dino_y = 45;
                    pulando = false;
                    velocidade_y = 0;
                }
            }
            obstaculo_x -= 3; // Velocidade do obstáculo
            if (obstaculo_x < 0) {
                obstaculo_x = 120; // Reinicia após sair da tela
            }

            if (verificar_colisao()) {
                jogo_rodando = false;
            }
        } else {
            // Quando o jogo não está rodando (GAME OVER)
            if (!som_tocado) {
                tocar_buzzer(500, 150); // Toca o buzzer apenas uma vez
                som_tocado = true; // Marca que o som já foi tocado
            }
            intensity = 0;
            ssd1306_draw_string(&ssd, "GAME OVER", 25, 10);
            draw_raposaGameOver(&ssd,50,20);
            set_led_intensity(led_slice_num, intensity);
            desenho_pio(desenho4, valor_led, pio, sm, r, g, b);
            gpio_put(LED_RED, true); // Liga o LED vermelho
        }

        // Desenha o dinossauro e o obstáculo
        draw_dino_sprite(&ssd, 10, dino_y);  // Desenha o sprite na posição (10,45)
        draw_obstaculo(&ssd, obstaculo_x, 53);  // Desenha na posição (50,55)

        // Desenha a borda
        ssd1306_line(&ssd, 3, 3, 120, 3, cor);   // Linha superior
        ssd1306_line(&ssd, 3, 60, 120, 60, cor); // Linha inferior

        ssd1306_send_data(&ssd);

        sleep_ms(50);
    }

    return 0;
}