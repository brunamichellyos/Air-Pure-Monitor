# AirPure Monitor

![Arquitetura Conceitual do AirPure](https://raw.githubusercontent.com/brunamichellyos/Air-Pure-Monitor/main/arquitetura_diagrama_conceitual_projeto_airpure.png)

**AirPure Monitor** é um sistema IoT para monitoramento em tempo real da qualidade do ar interno.  
Ele utiliza um ESP32 para coletar dados de temperatura, umidade, CO₂ e COVT, transmite via MQTT para o ThingSpeak (onde é calculado o THI e gerados dashboards) e dispara alertas automáticos por Telegram, Slack ou X quando valores críticos são detectados.

---

## Funcionalidades principais

- **Aquisição e processamento local** com ESP32 + DHT22, MH-Z14A e CCS811  
- **Transmissão MQTT** (QoS 1) para escalabilidade e confiabilidade  
- **Visualização e análises** no ThingSpeak + MATLAB (THI automático)  
- **Notificações em tempo real** via HTTP para Telegram, Slack e X  

