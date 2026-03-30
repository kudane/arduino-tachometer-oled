#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- 1. ตั้งค่าจอ OLED ---
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_ADDRESS = 0x3C;
const int OLED_RESET = -1;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- 2. ตั้งค่าการวัดรอบเครื่อง (Interrupt) ---
const byte PULSE_PIN = 2; // ขารับสัญญาณจาก Optocoupler (D2)
volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;
const int pulsesPerRevolution = 2; // จำนวนพัลส์ต่อ 1 รอบ (ค่าเริ่มต้นสำหรับ 4 สูบ)

// --- 3. ตั้งค่าระบบ Smoothing (ตัวเลขสมูท) ---
float smoothedRpm = 0;
const float smoothingFactor = 0.2; // ปรับความหนืด (0.1 ถึง 0.3)

// --- 4. ตั้งค่าระบบ UI และ Timer ---
unsigned long previousBlinkMillis = 0;   
const long blinkInterval = 100;     // ความเร็วกระพริบ Shift Light (ms)
bool isBlinkVisible = true;
unsigned long lastDrawTime = 0;     // ตัวจับเวลาสำหรับวาดหน้าจอ

// ==========================================
// Interrupt Service Routine (ISR)
// ทำงานทันทีเมื่อมีสัญญาณพัลส์เข้ามาที่ขา D2
// ==========================================
void pulseISR() {
  unsigned long currentTime = micros();
  unsigned long interval = currentTime - lastPulseTime;
  
  // Debounce: กรองสัญญาณขยะที่เร็วกว่า 2000us (ประมาณ 15,000 RPM)
  if (interval > 2000) { 
    pulseInterval = interval;
    lastPulseTime = currentTime;
  }
}

// ==========================================
// ฟังก์ชันตั้งค่าเริ่มต้น
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // เริ่มต้นจอ OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) { 
    Serial.println(F("OLED allocation failed"));
    for(;;); 
  }
  display.clearDisplay();
  display.display();

  // ตั้งค่าขา Interrupt และเปิด Pull-up ภายใน
  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), pulseISR, FALLING);
}

// ==========================================
// ฟังก์ชัน UI: วาด Header แจ้งเตือน (Shift Light)
// ==========================================
void displayHeader(int rpm, bool blinkState) {
  display.setTextSize(1);
  if (rpm > 3800) {
    if (blinkState) {
      display.setTextColor(BLACK, WHITE); // Invert: พื้นสว่าง ตัวหนังสือดำ
      display.setCursor(31, 0); 
      display.print("   SHIFT   "); 
    }
  } else {
    display.setTextColor(WHITE); 
    display.setCursor(34, 0); 
    display.print("x1000r/min"); 
  }
}

// ==========================================
// ฟังก์ชัน UI: วาดตัวเลข RPM
// ==========================================
void displayRPM(int rpm) {
  display.setTextSize(4);
  display.setTextColor(WHITE); // คืนค่าสีปกติ
  
  int cursorX = 16;
  if (rpm < 1000) cursorX = 28; 
  if (rpm < 100)  cursorX = 40;  
  if (rpm < 10)   cursorX = 52;   
  
  display.setCursor(cursorX, 18); 
  display.print(rpm); 
}

// ==========================================
// ฟังก์ชัน UI: วาดแถบวัดรอบด้านล่าง (Dynamic Color)
// ==========================================
void displayIndicator(int rpm) {
  int maxSteps = 10;
  // เทียบสัดส่วนรอบเครื่อง (สูงสุดที่ 6500 RPM ตามสเปกเดิม)
  int activeSteps = map(rpm, 0, 6500, 0, maxSteps);
  if (activeSteps > maxSteps) activeSteps = maxSteps;
  if (activeSteps < 0) activeSteps = 0;

  // 1. เช็คเงื่อนไขเพื่อกำหนดสีของหลอดไฟตามรอบเครื่องปัจจุบัน
  uint8_t barColor;
  if (rpm <= 2500) {
    barColor = 28;   // สีเขียว
  } else if (rpm <= 3800) {
    barColor = 244;  // สีเหลืองอำพัน
  } else {
    barColor = 224;  // สีแดง
  }

  // 2. วาดหลอดไฟลงบนจอ
  for (int i = 0; i < maxSteps; i++) {
    int x = 10 + (i * 24); // จัดระยะห่างของแต่ละช่องให้พอดีจอ 4.3 นิ้ว
    
    if (i < activeSteps) {
      // ระบายสีทึบตามเงื่อนไข (Dynamic Color)
      display.fillRect(x, 200, 20, 20, barColor); 
    } else {
      // ช่องที่รอบยังไม่ถึง ให้วาดแค่กรอบสีเทาเข้มเพื่อความสวยงาม
      display.drawRect(x, 200, 20, 20, 73); 
    }
  }
}

// ==========================================
// Main Flow (Controller)
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  // 1. สลับสถานะกระพริบ Shift Light (ทุกๆ 100ms)
  if (currentMillis - previousBlinkMillis >= blinkInterval) {
    previousBlinkMillis = currentMillis;
    isBlinkVisible = !isBlinkVisible; 
  }

  // 2. ควบคุมความเร็วในการวาดหน้าจอ (ประมาณ 33 FPS) ไม่ให้กินแรงบอร์ด
  if (currentMillis - lastDrawTime >= 30) {
    lastDrawTime = currentMillis;

    // --- ดึงข้อมูลจาก Interrupt อย่างปลอดภัย ---
    noInterrupts(); 
    unsigned long currentInterval = pulseInterval;
    unsigned long timeSinceLastPulse = micros() - lastPulseTime;
    interrupts();   

    int rawRpm = 0;

    // --- คำนวณ Raw RPM ---
    // ถ้าไม่มีพัลส์มาเกิน 1 วินาที ให้ถือว่าเครื่องดับ
    if (timeSinceLastPulse < 1000000) {
      if (currentInterval > 0) {
        rawRpm = (60000000UL / currentInterval) / pulsesPerRevolution;
      }
    }

    // --- เข้าสมการ Smoothing ---
    if (rawRpm == 0 || rawRpm < 300) {
      smoothedRpm = 0; 
    } else {
      smoothedRpm = (smoothingFactor * rawRpm) + ((1.0 - smoothingFactor) * smoothedRpm);
    }

    int finalDisplayRpm = (int)smoothedRpm;

    // --- สั่งวาดหน้าจอ ---
    display.clearDisplay();
    displayHeader(finalDisplayRpm, isBlinkVisible);      
    displayRPM(finalDisplayRpm);
    displayIndicator(finalDisplayRpm);
    display.display();
  }
}
