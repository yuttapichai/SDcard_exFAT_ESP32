#include <SPI.h>
#include <SdFat.h>

// ---- SPI pin ----
#define PIN_MOSI 23
#define PIN_MISO 19
#define PIN_SCK  18
#define PIN_CS    5

// ใช้ SdExFat สำหรับ exFAT volumes
SdExFat   sd;
ExFatFile file;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  Serial.println("\n=== ESP32 + SdFat exFAT Test for SD Card 64GB ===");
  
  // กำหนด SPI pins
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  
  // เริ่ม SD ด้วย DEDICATED SPI ที่ 10 MHz (สำหรับความเสถียร)
  SdSpiConfig cfg(PIN_CS, DEDICATED_SPI, SD_SCK_MHZ(10));
  
  Serial.print("กำลังเชื่อมต่อ exFAT volume ... ");
  if (!sd.begin(cfg)) {
    Serial.println("ล้มเหลว!");
    Serial.println("คำแนะนำ:");
    Serial.println("- ตรวจสอบให้แน่ใจว่าการ์ดถูกจัดรูปแบบเป็น exFAT");
    Serial.println("- ตรวจสอบขา CS และการต่อสาย (ใช้ 3.3V เท่านั้น)");
    Serial.println("- ลองใช้ SPI clock ต่ำกว่า เช่น 4-8 MHz");
    Serial.println("- ตรวจสอบว่าการ์ดใส่อย่างแน่นหนา");
    return;
  }
  Serial.println("สำเร็จ!");
  
  // ตรวจสอบประเภทไฟล์ระบบ (64 หมายถึง exFAT ใน SdFat)
  uint8_t vtype = sd.fatType();
  Serial.printf("ประเภท Volume: %u (คาดหวัง 64 สำหรับ exFAT)\n", vtype);
  
  if (vtype != 64) {
    Serial.println("คำเตือน: ไม่ใช่ exFAT filesystem!");
  }
  
  // แสดงข้อมูลการ์ด
  printCardInfo();
  
  // แสดงรายการไฟล์ในโฟลเดอร์หลัก
  Serial.println("\n=== รายการไฟล์ในโฟลเดอร์หลัก ===");
  listFiles();
  
  // ทดสอบการเขียนและอ่านไฟล์
  testFileOperations();
}

void loop() {
  delay(5000);
  
}

void printCardInfo() {
  Serial.println("\n=== ข้อมูลการ์ด ===");
  
  // ขนาดการ์ด
  uint64_t cardSize = sd.card()->sectorCount() * 512ULL;
  Serial.printf("ขนาดการ์ด: %.2f GB\n", cardSize / 1000000000.0);
  
  // พื้นที่ว่าง
  uint64_t freeSpace = sd.freeClusterCount() * sd.bytesPerCluster();
  Serial.printf("พื้นที่ว่าง: %.2f GB\n", freeSpace / 1000000000.0);
  
  // พื้นที่ที่ใช้แล้ว
  uint64_t usedSpace = cardSize - freeSpace;
  Serial.printf("พื้นที่ที่ใช้: %.2f GB\n", usedSpace / 1000000000.0);
}

void listFiles() {
  ExFatFile root;
  if (!root.open("/")) {
    Serial.println("ไม่สามารถเปิดโฟลเดอร์หลักได้");
    return;
  }
  
  ExFatFile entry;
  char name[256];
  int fileCount = 0;
  
  while (entry.openNext(&root, O_RDONLY)) {
    entry.getName(name, sizeof(name));
    
    Serial.printf("%3d. %-30s", ++fileCount, name);
    
    if (entry.isDir()) {
      Serial.print(" [โฟลเดอร์]");
    } else {
      uint64_t size = entry.fileSize();
      if (size < 1024) {
        Serial.printf(" %llu B", size);
      } else if (size < 1024 * 1024) {
        Serial.printf(" %.1f KB", size / 1024.0);
      } else if (size < 1024 * 1024 * 1024) {
        Serial.printf(" %.1f MB", size / (1024.0 * 1024.0));
      } else {
        Serial.printf(" %.1f GB", size / (1024.0 * 1024.0 * 1024.0));
      }
    }
    Serial.println();
    
    entry.close();
    
    // จำกัดจำนวนไฟล์ที่แสดง
    if (fileCount >= 50) {
      Serial.println("... (แสดงเพียง 50 รายการแรก)");
      break;
    }
  }
  
  root.close();
  Serial.printf("พบไฟล์/โฟลเดอร์ทั้งหมด: %d รายการ\n", fileCount);
}

void testFileOperations() {
  Serial.println("\n=== ทดสอบการเขียนและอ่านไฟล์ ===");
  
  // เขียนไฟล์ทดสอบ
  if (file.open("test_esp32.txt", O_CREAT | O_WRITE)) {
    Serial.print("กำลังเขียนไฟล์ทดสอบ... ");
    
    // ใช้ write() สำหรับเขียนข้อมูล
    const char* testText1 = "=== ESP32 SD Card Test ===\n";
    file.write(testText1, strlen(testText1));
    
    String timeStr = "เวลาที่สร้างไฟล์: " + String(millis()) + " ms\n";
    file.write(timeStr.c_str(), timeStr.length());
    
    const char* testText2 = "ขนาดการ์ด: 64GB\n";
    file.write(testText2, strlen(testText2));
    
    const char* testText3 = "ระบบไฟล์: exFAT\n";
    file.write(testText3, strlen(testText3));
    
    const char* testText4 = "ESP32 สามารถเขียนไฟล์ภาษาไทยได้\n";
    file.write(testText4, strlen(testText4));
    
    // เขียนข้อมูลแบบไบนารี่
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    file.write(testData, sizeof(testData));
    
    file.close();
    Serial.println("สำเร็จ!");
  } else {
    Serial.println("ล้มเหลวในการเขียนไฟล์!");
    return;
  }
  
  // อ่านไฟล์ทดสอบ
  if (file.open("test_esp32.txt", O_READ)) {
    Serial.println("เนื้อหาไฟล์ที่อ่านได้:");
    Serial.println("------------------------");
    
    // อ่านไฟล์ทีละไบต์
    while (file.available()) {
      int c = file.read();
      if (c >= 0) {
        Serial.print((char)c);
      }
    }
    
    file.close();
    Serial.println("\n------------------------");
    Serial.println("อ่านไฟล์สำเร็จ!");
  } else {
    Serial.println("ล้มเหลวในการอ่านไฟล์!");
  }
}

void readSpecificFile(const char* filename) {
  Serial.printf("\n=== อ่านไฟล์: %s ===\n", filename);
  
  if (file.open(filename, O_READ)) {
    Serial.printf("ขนาดไฟล์: %llu bytes\n", file.fileSize());
    
    // อ่านไฟล์ทีละบรรทัด
    int lineCount = 0;
    String line = "";
    
    while (file.available() && lineCount < 10) { // จำกัด 10 บรรทัดแรก
      int c = file.read();
      if (c >= 0) {
        char ch = (char)c;
        if (ch == '\n') {
          Serial.printf("บรรทัด %d: %s\n", ++lineCount, line.c_str());
          line = "";
        } else if (ch != '\r') {
          line += ch;
        }
      }
    }
    
    // แสดงบรรทัดสุดท้ายถ้ามี
    if (line.length() > 0) {
      Serial.printf("บรรทัด %d: %s\n", ++lineCount, line.c_str());
    }
    
    if (file.available()) {
      Serial.println("... (มีข้อมูลเพิ่มเติม)");
    }
    
    file.close();
  } else {
    Serial.printf("ไม่สามารถเปิดไฟล์ %s ได้\n", filename);
  }
}

// ฟังก์ชันสำหรับการจัดการข้อผิดพลาด
void handleSDError() {
  Serial.println("\nเกิดข้อผิดพลาดกับ SD Card");
  Serial.println("กำลังลองเชื่อมต่อใหม่...");
  
  // ลองเชื่อมต่อใหม่
  SdSpiConfig cfg(PIN_CS, DEDICATED_SPI, SD_SCK_MHZ(8)); // ใช้ความเร็วต่ำกว่า
  if (sd.begin(cfg)) {
    Serial.println("เชื่อมต่อใหม่สำเร็จ!");
  } else {
    Serial.println("ยังไม่สามารถเชื่อมต่อได้");
  }
}

// ฟังก์ชันเสริมสำหรับอ่านไฟล์ขนาดใหญ่แบบแบ่งส่วน
void readLargeFileInChunks(const char* filename, size_t chunkSize = 512) {
  Serial.printf("\n=== อ่านไฟล์ขนาดใหญ่: %s ===\n", filename);
  
  if (file.open(filename, O_READ)) {
    uint64_t totalSize = file.fileSize();
    Serial.printf("ขนาดไฟล์: %llu bytes\n", totalSize);
    
    uint8_t buffer[chunkSize];
    uint64_t bytesRead = 0;
    
    while (file.available() && bytesRead < 2048) { // อ่านแค่ 2KB แรก
      size_t bytesToRead = min(chunkSize, (size_t)(totalSize - bytesRead));
      bytesToRead = min(bytesToRead, (size_t)2048 - (size_t)bytesRead);
      
      int actualRead = file.read(buffer, bytesToRead);
      if (actualRead <= 0) break;
      
      Serial.printf("Chunk %d (%d bytes): ", (int)(bytesRead/chunkSize) + 1, actualRead);
      for (int i = 0; i < actualRead && i < 50; i++) { // แสดงแค่ 50 ไบต์แรกของแต่ละ chunk
        if (buffer[i] >= 32 && buffer[i] <= 126) {
          Serial.print((char)buffer[i]);
        } else {
          Serial.printf("[%02X]", buffer[i]);
        }
      }
      if (actualRead > 50) Serial.print("...");
      Serial.println();
      
      bytesRead += actualRead;
    }
    
    file.close();
    Serial.printf("อ่านทั้งหมด: %llu bytes\n", bytesRead);
  } else {
    Serial.printf("ไม่สามารถเปิดไฟล์ %s ได้\n", filename);
  }
}

// ฟังก์ชันสำหรับสร้างไฟล์ทดสอบขนาดใหญ่
void createLargeTestFile(const char* filename, size_t sizeKB) {
  Serial.printf("\n=== สร้างไฟล์ทดสอบขนาดใหญ่: %s (%zu KB) ===\n", filename, sizeKB);
  
  if (file.open(filename, O_CREAT | O_WRITE)) {
    const char* testPattern = "0123456789ABCDEF";
    size_t patternLen = strlen(testPattern);
    size_t totalBytes = sizeKB * 1024;
    size_t bytesWritten = 0;
    
    Serial.print("กำลังเขียน");
    while (bytesWritten < totalBytes) {
      size_t bytesToWrite = min(patternLen, totalBytes - bytesWritten);
      int written = file.write(testPattern, bytesToWrite);
      if (written <= 0) break;
      
      bytesWritten += written;
      
      // แสดงความคืบหน้า
      if (bytesWritten % (sizeKB * 102) == 0) { // ทุกๆ 10%
        Serial.print(".");
      }
    }
    
    file.close();
    Serial.printf("\nเขียนไฟล์สำเร็จ: %zu bytes\n", bytesWritten);
  } else {
    Serial.printf("ไม่สามารถสร้างไฟล์ %s ได้\n", filename);
  }
}

// ฟังก์ชันแสดงข้อมูลรายละเอียดของไฟล์
void showFileDetails(const char* filename) {
  Serial.printf("\n=== รายละเอียดไฟล์: %s ===\n", filename);
  
  ExFatFile testFile;
  if (testFile.open(filename, O_READ)) {
    Serial.printf("ขนาด: %llu bytes\n", testFile.fileSize());
    Serial.printf("ตำแหน่ง: %llu\n", testFile.curPosition());
    Serial.printf("Available: %d\n", testFile.available());
    
    // แสดงข้อมูล hex ของ 32 bytes แรก
    Serial.print("เนื้อหา 32 bytes แรก (HEX): ");
    for (int i = 0; i < 32 && testFile.available(); i++) {
      int b = testFile.read();
      if (b >= 0) {
        Serial.printf("%02X ", b);
      }
    }
    Serial.println();
    
    testFile.close();
  } else {
    Serial.printf("ไม่สามารถเปิดไฟล์ %s ได้\n", filename);
  }
}