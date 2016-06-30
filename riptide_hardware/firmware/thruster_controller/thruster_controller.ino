#include <Wire.h>
#include <ros.h>
#include <riptide_msgs/PwmStamped.h>
#include <riptide_msgs/Thrusters.h>

// Thrusters off!
int STOP = 1500;
// Checksum size
int ONE_PACKET = 10;
int FIVE_BYTES = 5;
// Addresses
int ESC_BOARD[] = {1, 2, 4, 8, 16};

// Function prototypes
int16_t valid(int16_t pwm);
void callback(const riptide_msgs::PwmStamped &cmd);

// ROS is the best
ros::NodeHandle nh;
riptide_msgs::Thrusters state;
ros::Publisher state_pub("state/esc", &state);
ros::Subscriber<riptide_msgs::PwmStamped> cmd_sub("command/pwm", &callback);

void setup()
{
  Wire.begin();

  nh.initNode();
  nh.advertise(state_pub);
  nh.subscribe(cmd_sub);

  // Surge LEDs
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  // Heave LEDs
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  // Sway LEDs
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
}

void loop()
{
  // Check msgs for callback
  nh.spinOnce();
}

// Production ready function
void callback(const riptide_msgs::PwmStamped &cmd)
{
  // Create local checksums
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.surge_port_hi >> 8) ^ (cmd.pwm.surge_port_hi);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.surge_stbd_hi >> 8) ^ (cmd.pwm.surge_stbd_hi);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.surge_port_lo >> 8) ^ (cmd.pwm.surge_port_lo);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.surge_stbd_lo >> 8) ^ (cmd.pwm.surge_stbd_lo);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.surge_sway_fwd >> 8) ^ (cmd.pwm.surge_sway_fwd);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.surge_sway_aft >> 8) ^ (cmd.pwm.surge_sway_aft);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.heave_port_fwd >> 8) ^ (cmd.pwm.heave_port_fwd);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.heave_stbd_fwd >> 8) ^ (cmd.pwm.heave_stbd_fwd);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.heave_port_aft >> 8) ^ (cmd.pwm.heave_port_aft);
  state.surge_port_hi.checksum = (uint8_t)(cmd.pwm.heave_stbd_aft >> 8) ^ (cmd.pwm.heave_stbd_aft);

  //
  // Write PWM data & request remote checksums
  //

  Wire.beginTransmission(ESC_BOARD[0]);
  Wire.write(valid(cmd.pwm.surge_port_hi) >> 8);
  Wire.write(valid(cmd.pwm.surge_port_hi));
  Wire.write(valid(cmd.pwm.surge_stbd_hi) >> 8);
  Wire.write(valid(cmd.pwm.surge_stbd_hi));
  Wire.endTransmission();

  Wire.requestFrom(ESC_BOARD[0], ONE_PACKET);
  state.surge_port_hi.checksum ^= Wire.read();
  state.surge_stbd_hi.checksum ^= Wire.read();
  state.surge_port_hi.temp = (Wire.read() | Wire.read() << 8)*;
  state.surge_port_hi.current = ((Wire.read() | Wire.read() << 8)-96.0)/36.0;
  state.surge_stbd_hi.temp = Wire.read() | Wire.read() << 8;
  state.surge_stbd_hi.current = Wire.read() | Wire.read() << 8;

  Wire.beginTransmission(ESC_BOARD[1]);
  Wire.write(valid(cmd.pwm.surge_port_lo) >> 8);
  Wire.write(valid(cmd.pwm.surge_port_lo));
  Wire.write(valid(cmd.pwm.surge_stbd_lo) >> 8);
  Wire.write(valid(cmd.pwm.surge_stbd_lo));
  Wire.endTransmission();

  Wire.requestFrom(ESC_BOARD[1], ONE_PACKET);
  state.surge_port_lo.checksum ^= Wire.read();
  state.surge_stbd_lo.checksum ^= Wire.read();
  state.surge_port_lo.temp = Wire.read() | Wire.read() << 8;
  state.surge_port_lo.current = Wire.read() | Wire.read() << 8;
  state.surge_stbd_lo.temp = Wire.read() | Wire.read() << 8;
  state.surge_stbd_lo.current = Wire.read() | Wire.read() << 8;

  Wire.beginTransmission(ESC_BOARD[2]);
  Wire.write(valid(cmd.pwm.sway_fwd) >> 8);
  Wire.write(valid(cmd.pwm.sway_fwd));
  Wire.write(valid(cmd.pwm.sway_aft) >> 8);
  Wire.write(valid(cmd.pwm.sway_aft));
  Wire.endTransmission();

  Wire.requestFrom(ESC_BOARD[2], ONE_PACKET);
  state.sway_fwd.checksum ^= Wire.read();
  state.sway_aft.checksum ^= Wire.read();
  state.sway_fwd.temp = Wire.read() | Wire.read() << 8;
  state.sway_fwd.current = Wire.read() | Wire.read() << 8;
  state.sway_aft.temp = Wire.read() | Wire.read() << 8;
  state.sway_aft.current = Wire.read() | Wire.read() << 8;

  Wire.beginTransmission(ESC_BOARD[3]);
  Wire.write(valid(cmd.pwm.heave_port_fwd) >> 8);
  Wire.write(valid(cmd.pwm.heave_port_fwd));
  Wire.write(valid(cmd.pwm.heave_stbd_fwd) >> 8);
  Wire.write(valid(cmd.pwm.heave_stbd_fwd));
  Wire.endTransmission();

  Wire.requestFrom(ESC_BOARD[3], ONE_PACKET);
  state.heave_port_fwd.checksum ^= Wire.read();
  state.heave_stbd_fwd.checksum ^= Wire.read();
  state.heave_port_fwd.temp = Wire.read() | Wire.read() << 8;
  state.heave_port_fwd.current = Wire.read() | Wire.read() << 8;
  state.heave_stbd_fwd.temp = Wire.read() | Wire.read() << 8;
  state.heave_stbd_fwd.current = Wire.read() | Wire.read() << 8;

  Wire.beginTransmission(ESC_BOARD[4]);
  Wire.write(valid(cmd.pwm.heave_port_aft) >> 8);
  Wire.write(valid(cmd.pwm.heave_port_aft));
  Wire.write(valid(cmd.pwm.heave_stbd_aft) >> 8);
  Wire.write(valid(cmd.pwm.heave_stbd_aft));
  Wire.endTransmission();

  Wire.requestFrom(ESC_BOARD[4], ONE_PACKET);
  state.heave_port_aft.checksum ^= Wire.read();
  state.heave_stbd_aft.checksum ^= Wire.read();
  state.heave_port_aft.temp = Wire.read() | Wire.read() << 8;
  state.heave_port_aft.current = Wire.read() | Wire.read() << 8;
  state.heave_stbd_aft.temp = Wire.read() | Wire.read() << 8;
  state.heave_stbd_aft.current = Wire.read() | Wire.read() << 8;

  // Publish checksum results
  state_pub.publish(&state);

  // Red LEDs
  if(cmd.pwm.surge_port_hi != STOP) { digitalWrite(2, HIGH); }
  else {digitalWrite(2, LOW); }
  if(cmd.pwm.surge_stbd_hi != STOP) { digitalWrite(3, HIGH); }
  else {digitalWrite(3, LOW); }
  if(cmd.pwm.surge_port_lo != STOP) { digitalWrite(4, HIGH); }
  else {digitalWrite(4, LOW); }
  if(cmd.pwm.surge_stbd_lo != STOP) { digitalWrite(5, HIGH); }
  else {digitalWrite(5, LOW); }

  // Amber LEDs
  if(cmd.pwm.heave_port_fwd != STOP) { digitalWrite(6, HIGH); }
  else {digitalWrite(6, LOW); }
  if(cmd.pwm.heave_stbd_fwd != STOP) { digitalWrite(7, HIGH); }
  else {digitalWrite(7, LOW); }
  if(cmd.pwm.heave_port_aft != STOP) { digitalWrite(8, HIGH); }
  else {digitalWrite(8, LOW); }
  if(cmd.pwm.heave_stbd_aft != STOP) { digitalWrite(9, HIGH); }
  else {digitalWrite(9, LOW); }

  // Blue LEDs
  if(cmd.pwm.sway_fwd != STOP) { digitalWrite(10, HIGH); }
  else {digitalWrite(10, LOW); }
  if(cmd.pwm.sway_aft != STOP) { digitalWrite(11, HIGH); }
  else {digitalWrite(11, LOW); }
}

// Ensure 1100 <= pwm <= 1900  state.surge_port_hi.checksum ^= Wire.read();
  state.surge_stbd_hi.checksum ^= Wire.read();
int16_t valid(int16_t pwm)
{
  pwm = pwm > 1900 ? 1900 : pwm;
  pwm = pwm < 1100 ? 1100 : pwm;
  return pwm;
}