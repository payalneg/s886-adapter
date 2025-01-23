#include <SoftwareSerial.h>
#include <Arduino.h>

#define RX_PIN 10
#define TX_PIN 11

#define BRAKE_BY_REGEN_OUT_PIN 7
#define BRAKE_IN_PIN 6
#define BRAKE_BY_MOTOR_OUT_PIN 5
#define CRUISE_PIN 4

#define SPEED_TEST_PIN 3
#define SPEED_INPUT_PIN 2
#define THROTTLE_OUT_PIN 9
SoftwareSerial mySerial(RX_PIN, TX_PIN);

struct 
{
  uint8_t drive_mode;
  uint8_t gear;
  uint8_t direct_start_kick_to_start;
  uint8_t magnets_count;
  uint16_t wheel_diameter;
  uint8_t pedal_assist_sensitive;
  uint8_t pedal_assist_starting_intensity;
  uint8_t speed_limit;
  uint8_t current_limit;
  uint16_t voltage_battery_cutoff;
  uint16_t throttle;
  uint8_t magnets_number_pedal_assist;
  uint8_t cruise_control;
  uint8_t e_brake;
} parameters;

uint8_t index = 0;
#define RX_BUFFER_SIZE 20
#define TX_BUFFER_SIZE 14
uint8_t RX_Buffer[RX_BUFFER_SIZE];
uint8_t TX_Buffer[TX_BUFFER_SIZE] = {0x02, 0x0E, 0x01, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0xFF, 0x0};
unsigned long last_byte_time;
unsigned long lost_connection_time;

bool check_crc (uint8_t * buf, uint8_t size)
{
  uint8_t crc=0;
  for (uint8_t i = 0; i < size - 1; i++)
  {
    crc^=buf[i];
  }
  return (crc == buf[size-1]);
}

void calc_crc (uint8_t * buf, uint8_t size)
{
  uint8_t crc=0;
  for (uint8_t i = 0; i < size - 1; i++)
  {
    crc^=buf[i];
  }
  buf[size-1] = crc;
}

#define PULSES_TO_CALC 10

volatile unsigned long firstPulseTime = 0; 
volatile int pulseIndex = 0;             
volatile unsigned long diffTimePulses = 0;
volatile unsigned long lastPulseTime = 0; 

unsigned int calculate_impulses(uint16_t speed_kmh_x10, uint16_t wheel_diameter) {

    float circumference_m = M_PI * (float)wheel_diameter * 0.00254  ;
    float speed_mps = (float)speed_kmh_x10 / 36.0;
    float rotations_per_second = speed_mps / circumference_m;
    //float impulses = 600.0f / rotations_per_second;
    float impulses = 1000.0f / rotations_per_second;

    return (unsigned int)impulses;
}

uint32_t calculate_speed() {
    uint16_t currentSpeed;

    if (diffTimePulses == 0)
    {
        currentSpeed = 0;
        return 0;
    }
    float diffDistance = (parameters.wheel_diameter * 2.54 * PI * PULSES_TO_CALC) / parameters.magnets_count;    
    float speed = diffDistance * 3.6  * 1000/ diffTimePulses;
    if (speed < 1)
        speed = 0;
    if (speed > 99)
      speed = 99;

    if ((micros() - firstPulseTime) > 500000)
    {
      speed = 0;
    }

    currentSpeed = speed*10;

    //diffTimePulses = 0;
    return currentSpeed;
}

void wheel_interrupt() {
    unsigned long currentTime;
    currentTime = micros();

    if ((currentTime - lastPulseTime) < 1000)
        return;
        
    if (lastPulseTime > 0) {
        // Store the time of the pulse in the circular buffer        
        if (++pulseIndex == PULSES_TO_CALC)
        {
            diffTimePulses = currentTime - firstPulseTime;
            firstPulseTime = currentTime;
            pulseIndex = 0;        
        }        
    }
    lastPulseTime = currentTime;
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(SPEED_INPUT_PIN), wheel_interrupt, CHANGE);
    pinMode(SPEED_TEST_PIN, OUTPUT);
    pinMode(CRUISE_PIN, OUTPUT);
    pinMode(BRAKE_BY_MOTOR_OUT_PIN, OUTPUT);
    pinMode(BRAKE_BY_REGEN_OUT_PIN, OUTPUT);
    pinMode(BRAKE_IN_PIN, INPUT_PULLUP);
    
    tone(SPEED_TEST_PIN, 59); 
    pinMode(THROTTLE_OUT_PIN, OUTPUT);
    Serial.begin(115200);
    mySerial.begin(9600);

    TCCR1A = 0;
    TCCR1B = 0;

    TCCR1A |= (1 << WGM10);
    TCCR1B |= (1 << WGM12); 
    TCCR1A |= (1 << COM1A1);

    TCCR1B |= (1 << CS11);

}

void loop() {
  if (mySerial.available())
  {
      RX_Buffer[index] = mySerial.read();
      last_byte_time = millis();
      if (++index == RX_BUFFER_SIZE)
      {
        index = 0;
        if (check_crc(RX_Buffer, RX_BUFFER_SIZE))
        {
          lost_connection_time=millis();
          parameters.drive_mode = RX_Buffer[3];
          parameters.gear = RX_Buffer[4];
          parameters.direct_start_kick_to_start = RX_Buffer[5];
          parameters.magnets_count = RX_Buffer[6];
          parameters.wheel_diameter = RX_Buffer[7];
          parameters.wheel_diameter <<= 8;
          parameters.wheel_diameter |= RX_Buffer[8];
          parameters.pedal_assist_sensitive = RX_Buffer[9];
          parameters.pedal_assist_starting_intensity = RX_Buffer[10];
          parameters.speed_limit = RX_Buffer[12];
          parameters.current_limit = RX_Buffer[13];
          parameters.voltage_battery_cutoff = RX_Buffer[14];
          parameters.voltage_battery_cutoff <<= 8;
          parameters.voltage_battery_cutoff |= RX_Buffer[15];

          parameters.throttle = RX_Buffer[16];
          parameters.throttle <<= 8;
          parameters.throttle |= RX_Buffer[17];

          parameters.magnets_number_pedal_assist = RX_Buffer[18] & 0x0F;
          parameters.cruise_control = ((RX_Buffer[18] & 0x40) == 0x40);
          parameters.e_brake = ((RX_Buffer[18] & 0x80) == 0x80);

          OCR1A = map(parameters.throttle, 0, 1023, 60, 255); 

          uint16_t speed = calculate_speed();
          uint16_t pulses = calculate_impulses(speed, parameters.wheel_diameter);
          if (speed == 0)
            pulses = 0x1770;
          TX_Buffer[8] = (pulses>>8) & 0xFF;
          TX_Buffer[9] = (pulses) & 0xFF;

          calc_crc(TX_Buffer,TX_BUFFER_SIZE);
          mySerial.write(TX_Buffer,TX_BUFFER_SIZE);
          /*
          for (uint8_t i = 0; i < RX_BUFFER_SIZE; i++)
          {
            
            char temp[10];
            sprintf(temp,"%02X ",RX_Buffer[i]);
            Serial.print(temp);
          }
          Serial.println();
          */
        }
      }
  }

  if (last_byte_time != 0)
  {
    if ((millis() - last_byte_time) > 50)
    {
      index = 0;
      last_byte_time = 0;
    }
  }

  if ((millis() - lost_connection_time) > 1000)
  {
    OCR1A = 0;
  }

  if (parameters.cruise_control)
  {
    digitalWrite(CRUISE_PIN, false);
  }
  else
  {
    digitalWrite(CRUISE_PIN, true);
  }

  if (!digitalRead(BRAKE_IN_PIN))
  {
    if (!parameters.e_brake)
    {
      digitalWrite(BRAKE_BY_MOTOR_OUT_PIN, true);
      digitalWrite(BRAKE_BY_REGEN_OUT_PIN, false);
    }
    else
    {
      digitalWrite(BRAKE_BY_MOTOR_OUT_PIN, false);
      digitalWrite(BRAKE_BY_REGEN_OUT_PIN, true);
    }
  }
  else
  {
    digitalWrite(BRAKE_BY_MOTOR_OUT_PIN, true);
    digitalWrite(BRAKE_BY_REGEN_OUT_PIN, true);
  }

    static unsigned long last_print_time = 0;
    unsigned long current_time = millis();

    if (current_time - last_print_time >= 1000) {
        last_print_time = current_time;
        uint32_t speed = calculate_speed();
        Serial.print("Speed: ");
        Serial.print(speed);
        Serial.print(" wheel_diameter: ");
        Serial.print(parameters.wheel_diameter);
        Serial.print(" magnets_count: ");
        Serial.print(parameters.magnets_count);
        Serial.println("");
    }
    
}
