# Arduino Car Tachometer with OLED Display

> 💡 **Important Note:** > โค้ดทั้งหมดในโปรเจกต์นี้เกิดจากการ **vide code**

โปรเจกต์เกจวัดรอบเครื่องยนต์ (RPM) สำหรับรถยนต์ แสดงผลผ่านหน้าจอ OLED 0.96 นิ้ว พร้อมระบบ Shift Light แจ้งเตือนเมื่อรอบเครื่องสูง และระบบกรองสัญญาณรบกวน (Smoothing Filter) ออกแบบมาให้ทำงานได้อย่างลื่นไหลและปลอดภัยต่อบอร์ดไมโครคอนโทรลเลอร์

## Features
- **Real-time RPM:** แสดงรอบเครื่องยนต์แบบตัวเลขขนาดใหญ่
- **Step Indicator:** แถบวัดรอบแบบ 10 ระดับด้านล่างจอ
- **Shift Light Warning:** แจ้งเตือนด้วยข้อความ "!! SHIFT !!" แบบกะพริบสลับสี (Invert & Blink) เมื่อรอบ > ุ3800 RPM
- **Smart Idle Cutoff:** ตัดค่าแสดงผลเป็น 0 ทันทีเมื่อรอบต่ำกว่า 500 RPM
- **EMA Smoothing:** กรองตัวเลขให้วิ่งขึ้นลงอย่างนุ่มนวล ไม่แกว่ง

---

## Hardware Specifications
- **Microcontroller:** Arduino Nano (5V / 16MHz)
- **Display:** 0.96" OLED Display (SSD1306, I2C, 128x64 pixels)
- **Signal Isolation:** PC817 (หรือ 4N35) Optocoupler
- **Protection Components:** - 1N4007 Diode (กันไฟย้อน)
  - 1kΩ - 2.2kΩ Resistor (ดรอปกระแส 12V)
- **Noise Filtering (Hardware Debounce):** - 100µF / 16V Electrolytic Capacitor (สำรองไฟให้จอ)
  - 0.1µF (104) Ceramic Capacitor x2 (กรองคลื่นแทรกความถี่สูง)
- **Power Supply:** 5V จาก USB Car Charger (เสียบเข้าพอร์ต USB ของ Arduino)

---

## Circuit Diagram / Wiring
![Circuit](https://github.com/kudane/arduino-tachometer-oled/blob/main/diagram.jpg)
![UI Demo](https://github.com/kudane/arduino-tachometer-oled/blob/main/image.JPG)

### 1. การต่อจอ OLED (I2C)
| OLED Pin | Arduino Nano Pin |
| :--- | :--- |
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

### 2. วงจรรับสัญญาณวัดรอบ (Optocoupler Isolation)
วงจรนี้ใช้สำหรับแปลงสัญญาณ 12V จากคอยล์/ECU รถยนต์ให้กลายเป็น 5V อย่างปลอดภัย

**ฝั่ง 12V (รถยนต์ ➡️ PC817)**
1. **Tacho Signal (12V):** ต่อเข้าขั้วบวก (Anode) ของไดโอด 1N4007
2. **ขั้วลบของไดโอด:** ต่อพ่วงกับตัวต้านทาน 1kΩ
3. **ตัวต้านทาน 1kΩ:** ต่อเข้า **Pin 1** ของ PC817
4. **Car GND (ตัวถังรถ):** ต่อเข้า **Pin 2** ของ PC817

**ฝั่ง 5V (PC817 ➡️ Arduino)**
1. **Pin 3 ของ PC817:** ต่อเข้า **GND** ของ Arduino
2. **Pin 4 ของ PC817:** ต่อเข้า **D2** ของ Arduino (รับสัญญาณ Interrupt)

### 3. จุดเชื่อมต่อ Capacitor (ป้องกันภาพล้มและตัวเลขรวน)
- **100µF & 0.1µF:** ต่อคร่อมระหว่างขา **5V** และ **GND** ของ Arduino 
- **0.1µF:** ต่อคร่อมระหว่างขา **D2** (หรือ Pin 4 ของ PC817) และ **GND** (เพื่อทำ Hardware Debounce)

---

## Libraries Required
โปรเจกต์นี้จำเป็นต้องติดตั้ง Library ผ่าน Arduino IDE Library Manager ดังนี้:
- `Adafruit_GFX.h`
- `Adafruit_SSD1306.h`
- `Wire.h` (มีมาให้ในตัวอยู่แล้ว)
