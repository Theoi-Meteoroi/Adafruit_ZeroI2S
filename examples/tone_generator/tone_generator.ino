// Arduino Zero / Feather M0 I2S audio tone generation example.
// Author: Tony DiCola
//
// Connect an I2S DAC or amp (like the MAX98357) to the Arduino Zero
// and play back simple sine, sawtooth, triangle, and square waves.
// Makes your Zero sound like a NES!
//
// NOTE: The I2S signal generated by the Zero does NOT have a MCLK /
// master clock signal.  You must use an I2S receiver that can operate
// without a MCLK signal (like the MAX98357).
//
// For an Arduino Zero / Feather M0 connect it to you I2S hardware as follows:
// - Digital 0 -> I2S LRCLK / FS (left/right / frame select clock)
// - Digital 1 -> I2S BCLK / SCLK (bit / serial clock)
// - Digital 9 -> I2S DIN / SD (data output)
// - Ground
//
// Depends on the Adafruit_ASFcore library from:
//   https://github.com/adafruit/adafruit_asfcore
//
// Released under a MIT license: https://opensource.org/licenses/MIT
#include "Adafruit_ASFCore.h"
#include "Adafruit_ZeroI2S.h"


#define SAMPLERATE_HZ 44100  // The sample rate of the audio.  Higher sample rates have better fidelity,
                             // but these tones are so simple it won't make a difference.  44.1khz is
                             // standard CD quality sound.

#define AMPLITUDE     5000   // Set the amplitude of generated waveforms.  This controls how loud
                             // the signals are, and can be any value from 0 to 65535.  Start with
                             // a low value like 5000 or less to prevent damaging speakers!

#define WAV_SIZE      256    // The size of each generated waveform.  The larger the size the higher
                             // quality the signal.  A size of 256 is more than enough for these simple
                             // waveforms.


// Define the frequency of music notes (from http://www.phy.mtu.edu/~suits/notefreqs.html):
#define C4_HZ      261.63
#define D4_HZ      293.66
#define E4_HZ      329.63
#define F4_HZ      349.23
#define G4_HZ      392.00
#define A4_HZ      440.00
#define B4_HZ      493.88

// Define a C-major scale to play all the notes up and down.
float scale[] = { C4_HZ, D4_HZ, E4_HZ, F4_HZ, G4_HZ, A4_HZ, B4_HZ, A4_HZ, G4_HZ, F4_HZ, E4_HZ, D4_HZ, C4_HZ };

// Store basic waveforms in memory.
int16_t sine[WAV_SIZE]     = {0};
int16_t sawtooth[WAV_SIZE] = {0};
int16_t triangle[WAV_SIZE] = {0};
int16_t square[WAV_SIZE]   = {0};

// Create I2S audio transmitter object.
Adafruit_ZeroI2S_TX i2s = Adafruit_ZeroI2S_TX();

// Little define to make the native USB port on the Arduino Zero / Zero feather
// the default for serial output.
#define Serial SerialUSB


void generateSine(uint16_t amplitude, int16_t* buffer, uint16_t length) {
  // Generate a sine wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  for (int i=0; i<length; ++i) {
    buffer[i] = uint16_t(float(amplitude)*sin(2.0*PI*(1.0/length)*i));
  }
}
void generateSawtooth(uint16_t amplitude, int16_t* buffer, uint16_t length) {
  // Generate a sawtooth signal that goes from -amplitude/2 to amplitude/2
  // and store it in the provided buffer of size length.
  float delta = float(amplitude)/float(length);
  for (int i=0; i<length; ++i) {
    buffer[i] = -(amplitude/2)+delta*i;
  }
}

void generateTriangle(uint16_t amplitude, int16_t* buffer, uint16_t length) {
  // Generate a triangle wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  float delta = float(amplitude)/float(length);
  for (int i=0; i<length/2; ++i) {
    buffer[i] = -(amplitude/2)+delta*i;
  }
    for (int i=length/2; i<length; ++i) {
    buffer[i] = (amplitude/2)-delta*(i-length/2);
  }
}

void generateSquare(uint16_t amplitude, int16_t* buffer, uint16_t length) {
  // Generate a square wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  for (int i=0; i<length/2; ++i) {
    buffer[i] = -(amplitude/2);
  }
    for (int i=length/2; i<length; ++i) {
    buffer[i] = (amplitude/2);
  }
}

void playWave(int16_t* buffer, uint16_t length, float frequency, float seconds) {
  // Play back the provided waveform buffer for the specified
  // amount of seconds.
  // First calculate how many samples need to play back to run
  // for the desired amount of seconds.
  uint32_t iterations = seconds*SAMPLERATE_HZ;
  // Then calculate the 'speed' at which we move through the wave
  // buffer based on the frequency of the tone being played.
  float delta = (frequency*length)/float(SAMPLERATE_HZ);
  // Now loop through all the samples and play them, calculating the
  // position within the wave buffer for each moment in time.
  for (uint32_t i=0; i<iterations; ++i) {
    uint16_t pos = uint32_t(i*delta) % length;
    int16_t sample = buffer[pos];
    // Duplicate the sample so it's sent to both the left and right channel.
    // It appears the order is right channel, left channel if you want to write
    // stereo sound.
    i2s.write(sample);
    i2s.write(sample);
  }
}

void setup() {
  // Configure serial port.
  Serial.begin(115200);
  Serial.println("Zero I2S Audio Tone Generator");

  // Initialize the I2S transmitter.
  if (!i2s.begin()) {
    Serial.println("Failed to initialize I2S transmitter!");
    while (1);
  }

  // Configure I2S transmitter for stereo 16-bit audio at the desired sampling rate.
  // Note it's important to use 2 channels for the MAX98357 I2S amp because it gets
  // confused with just 1 channel and plays incorrect sounding audio.  For mono sound
  // just use 2 channels and either duplicate or blank out the second channel sample.
  if (!i2s.configure(2, SAMPLERATE_HZ, 16)) {
    Serial.println("Failed to configure I2S transmitter for stereo 16-bit 44.1khz audio!");
    while (1);
  }

  // Generate waveforms.
  generateSine(AMPLITUDE, sine, WAV_SIZE);
  generateSawtooth(AMPLITUDE, sawtooth, WAV_SIZE);
  generateTriangle(AMPLITUDE, triangle, WAV_SIZE);
  generateSquare(AMPLITUDE, square, WAV_SIZE);
}

void loop() {
  Serial.println("Sine wave");
  for (int i=0; i<sizeof(scale)/sizeof(float); ++i) {
    // Play the note for a quarter of a second.
    playWave(sine, WAV_SIZE, scale[i], 0.25);
    // Pause for a tenth of a second between notes.
    delay(100);
  }
  Serial.println("Sawtooth wave");
  for (int i=0; i<sizeof(scale)/sizeof(float); ++i) {
    // Play the note for a quarter of a second.
    playWave(sawtooth, WAV_SIZE, scale[i], 0.25);
    // Pause for a tenth of a second between notes.
    delay(100);
  }
  Serial.println("Triangle wave");
  for (int i=0; i<sizeof(scale)/sizeof(float); ++i) {
    // Play the note for a quarter of a second.
    playWave(triangle, WAV_SIZE, scale[i], 0.25);
    // Pause for a tenth of a second between notes.
    delay(100);
  }
  Serial.println("Square wave");
  for (int i=0; i<sizeof(scale)/sizeof(float); ++i) {
    // Play the note for a quarter of a second.
    playWave(square, WAV_SIZE, scale[i], 0.25);
    // Pause for a tenth of a second between notes.
    delay(100);
  }
}
