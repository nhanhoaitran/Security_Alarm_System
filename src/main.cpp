#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <TimerOne.h>

// ---------------- CẤU HÌNH PHẦN CỨNG ----------------
#define PIR_PIN       2    // (1) Chân Ngắt ngoài 0 (INT0) - ƯU TIÊN CAO NHẤT
#define BUZZER_PIN    12
#define LED_PIN       13

void updateLCD(String line1, String line2);
void handleEntryDelay(char key);
void handlePasswordInput(char key);
void pirInterrupt();
void timerIsr();
void changePasswordRoutine(); 

// Cấu hình Keypad 4x4
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {11, 10, 9, 8}; 
byte colPins[COLS] = {7, 6, 5, 4}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Cấu hình LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// CẤU HÌNH LOGIC 
String CORRECT_PIN = "1234"; 
String currentInput = "";
volatile bool menuDisplayed = false; // Biến cờ hiệu giúp LCD không bị nháy

// Các trạng thái của hệ thống (FSM)
enum SystemState {
  DISARMED,     // Đã tắt báo động
  MENU,         // Menu chọn A hoặc B
  EXIT_DELAY,   // Đếm ngược 10s để ra khỏi nhà
  ARMED,        // Đang canh gác (chờ PIR)
  ENTRY_DELAY,  // (3) Delay vào: 30s chờ nhập pass
  ALARM         // Hú còi
};
volatile SystemState currentState = DISARMED;

// Các biến đếm thời gian
volatile unsigned long delayStartTime = 0; 
const unsigned long ENTRY_DURATION = 30000; // 30 giây Delay vào
const unsigned long EXIT_DURATION = 10000;  // 10 giây Delay ra (mới thêm)

// Biến cho Timer điều khiển LED/Còi
volatile int sirenFreq = 500;
volatile int sirenDirection = 10;
volatile bool ledState = LOW;

// ---------------- HÀM NGẮT (ISR) ----------------

// (1) Ngắt ngoài PIR: Ưu tiên cao nhất
void pirInterrupt() {
  if (currentState == ARMED) {
    currentState = ENTRY_DELAY;
    delayStartTime = millis(); // Đánh dấu thời điểm phát hiện
    menuDisplayed = false;     // Reset cờ hiệu hiển thị
  }
}

// (2) Ngắt Timer 1: Tạo chu kỳ nháy đèn và hú còi biến thiên
void timerIsr() {
  if (currentState == ALARM) {
    sirenFreq += sirenDirection;
    if (sirenFreq > 1500 || sirenFreq < 500) {
      sirenDirection = -sirenDirection; 
    }
    tone(BUZZER_PIN, sirenFreq);

    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
  }
}

// ---------------- HÀM SETUP & LOOP ----------------

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  
  menuDisplayed = false; // Kích hoạt vẽ màn hình đầu tiên

  // (1) Khởi tạo Ngắt ngoài PIR (Cạnh lên)
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), pirInterrupt, RISING);

  // (2) Khởi tạo Timer 1 (20ms)
  Timer1.initialize(20000);
  Timer1.attachInterrupt(timerIsr);
}

void loop() {
  char key = keypad.getKey();

  switch (currentState) {
    case DISARMED:
      if (!menuDisplayed) {
        updateLCD("SYSTEM SECURE", "Press * for MENU");
        menuDisplayed = true;
      }
      if (key == '*') {
        currentState = MENU;
        menuDisplayed = false;
      }
      break;

    case MENU:
      if (!menuDisplayed) {
        updateLCD("A - Activate", "B - Change Pass");
        menuDisplayed = true;
      }
      if (key == 'A') {
        currentState = EXIT_DELAY;
        delayStartTime = millis();
        menuDisplayed = false;
      } else if (key == 'B') {
        changePasswordRoutine(); // Gọi quy trình đổi mật khẩu
        currentState = DISARMED; // Đổi xong về lại màn hình chính
        menuDisplayed = false;
      } else if (key == '#') {
        currentState = DISARMED; // Phím thoát menu
        menuDisplayed = false;
      }
      break;

    case EXIT_DELAY:
      {
        unsigned long elapsedTime = millis() - delayStartTime;
        if (elapsedTime >= EXIT_DURATION) {
          // Hết 10s đếm ngược -> Bắt đầu canh gác
          currentState = ARMED;
          menuDisplayed = false;
          updateLCD("Alarm Activated!", ""); 
        } else {
          // Hiển thị đếm ngược trên LCD mỗi giây
          static unsigned long lastTick = 0;
          if (millis() - lastTick >= 500) {
            int timeLeft = (EXIT_DURATION - elapsedTime) / 1000;
            updateLCD("Alarm will be", "activated in " + String(timeLeft) + "s");
            lastTick = millis();
          }
        }
      }
      break;

    case ARMED:
      // Ở trạng thái này, Ngắt ngoài PIR (INT0) nắm toàn quyền.
      // Khi có người, PIR sẽ kích hoạt hàm pirInterrupt() ngay lập tức.
      break;

    case ENTRY_DELAY:
      // (3) Delay vào 30s
      handleEntryDelay(key);
      break;

    case ALARM:
      if (!menuDisplayed) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("!!! BAO DONG !!!");
        menuDisplayed = true;
      }
      handlePasswordInput(key);
      break;
  }
}

// HÀM XỬ LÝ PHỤ TRỢ

void handleEntryDelay(char key) {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - delayStartTime;
  
  if (elapsedTime >= ENTRY_DURATION) { // Quá 30s -> Báo động
    currentState = ALARM;
    currentInput = "";
    menuDisplayed = false;
    return;
  }

  static unsigned long lastLcdUpdate = 0;
  if (currentTime - lastLcdUpdate > 500) {
    int timeLeft = (ENTRY_DURATION - elapsedTime) / 1000;
    lcd.setCursor(0, 0);
    lcd.print("Nhap Pass: ");
    lcd.print(timeLeft);
    lcd.print("s  "); 
    lastLcdUpdate = currentTime;
  }
  handlePasswordInput(key);
}

void handlePasswordInput(char key) {
  if (key) {
    if (key >= '0' && key <= '9') {
      currentInput += key;
      lcd.setCursor(0, 1);
      lcd.print("PIN: ");
      for (unsigned int i = 0; i < currentInput.length(); i++) {
        lcd.print("*");
      }
    } 
    else if (key == '#') { 
      if (currentInput == CORRECT_PIN) {
        currentState = DISARMED;
        menuDisplayed = false;
        updateLCD("Pass Chinh Xac!", "He thong Da Tat");
        delay(2000); 
      } else {
        lcd.setCursor(0, 1);
        lcd.print("Sai Pass!      ");
        delay(1000); 
        lcd.setCursor(0, 1);
        lcd.print("PIN:            ");
      }
      currentInput = ""; 
    }
    else if (key == 'D') { 
      if (currentInput.length() > 0) {
        currentInput.remove(currentInput.length() - 1);
        lcd.setCursor(0, 1);
        lcd.print("PIN:            ");
        lcd.setCursor(5, 1);
        for (unsigned int i = 0; i < currentInput.length(); i++) lcd.print("*");
      }
    }
  }
}

// QUY TRÌNH ĐỔI MẬT KHẨU (Khóa chặn hệ thống cho đến khi hoàn thành)
void changePasswordRoutine() {
  String tempPassword = "";
  
  // BƯỚC 1: NHẬP PASS CŨ
  updateLCD("Current Password", ">");
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        tempPassword += key;
        lcd.print("*");
      } else if (key == '#') {
        if (tempPassword == CORRECT_PIN) {
          break; // Mật khẩu đúng, chuyển sang bước 2
        } else {
          updateLCD("Wrong! Try Again", "");
          delay(2000);
          return; // Sai thì thoát về màn hình chính
        }
      } else if (key == 'D') { // Phím D để Hủy
        return;
      }
    }
  }

  // BƯỚC 2: NHẬP PASS MỚI
  tempPassword = "";
  updateLCD("Set New Password", ">");
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        tempPassword += key;
        lcd.print(key); // Hiện mật khẩu mới để người dùng nhìn thấy
      } else if (key == '#') {
        if (tempPassword.length() > 0) { // Nếu có nhập chữ số
          CORRECT_PIN = tempPassword; // Lưu mật khẩu mới
          updateLCD("Password Changed", "Successfully!");
          delay(2000);
        }
        return; // Đổi xong thoát hàm
      } else if (key == 'D') { // Phím D để Hủy
        return; 
      }
    }
  }
}

void updateLCD(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}