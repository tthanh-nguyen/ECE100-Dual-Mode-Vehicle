### Dual-Mode Controlled Vehicle | *Team Leader*

* **Course:** Introduction to Electrical and Electronics Engineering (ECE100)
* **Description:** Led a student team to build a smart vehicle controlled by **3 microcontrollers** working together.
* **Key Responsibilities & Technical Stack:**
* Designed the system layout and managed the connection between hardware and software.
    * **Arduino A (Master):** Connected to a PC and programmed to send wireless control commands using the **nRF24L01 module (via SPI interface)**.
    * **Arduino B (Slave Receiver):** Programmed to receive wireless signals from Chip 1, then pass the command down to Chip 3 **via UART interface**.
    * **Arduino C (Motor Controller):** Received data from Chip 2 **via UART** to control the motors. Programmed 2 modes: **Manual** (steering, acceleration) and **Autonomous** (automatic obstacle avoidance).
