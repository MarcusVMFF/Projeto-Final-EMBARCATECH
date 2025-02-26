# 📊 **Projeto-Final-EMBARCATECH**

Projeto final para capacitação em Sistemas Embarcados TIC 37 - Embarcatech, este projeto consite em um jogo de plataforma rodado completamente pela BitDogLab e seus hardwares integrados, como LEDs, botões, display, matriz 5x5, microfone, entre outros.. 

---

## 🔎 **Objetivos**

O objetivo principal é demonstrar todos os conteudos aprendidos durante a capacitação, criando um projeto que consiga incluir todos, sendo feito usando unicamente a placa BitDogLab.
Este projeto utiliza os assuntos aprendidos durante a capacitação, manipulação de botões com rotinas de interrupção, LED GPIO, LED PWM, joystick, buzzer, microfone por conversor analogico digital, matriz 5x5 por .PIO, display SSD 1306 por comunicação via I2C, 

---

## 🎥 **Demonstração**

[Ver Vídeo do Projeto](https://drive.google.com/file/d/1ucnKRTTU2zMTeUkhbuDoAb8zPxELnhie/view?usp=sharing)

---

## 🛠️ **Tecnologias Utilizadas**

- **Linguagem de Programação:** C / CMake
- **Placas Microcontroladoras:**
  - BitDogLab
  - Pico W
  - Simulador Wowki
---

## 📖 **Como Utilizar**

- O jogo inicia com uma pequena animação
- Enquanto o jogo está rodando, o LED na cor verde é acesso com 50 de intensidade por PWM
- O botão A faz o personagem pular
- O botão B habilita o modo controle por voz, em que o peersonagem voa conforme o som é recebido pelo microfone
- Ao colidir com um obstaculo, a tela Game Over aparece, junto acende a matriz 5x5 e o LED vermelho, também faz um curto som pelo buzzer
- O botão do joystick reinicia o jogo
---
