/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2022 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
 // HomeSpan Addressable RGB LED Example

// Demonstrates use of HomeSpan Pixel Class that provides for control of single-wire
// addressable RGB LEDs, such as the SK68xx or WS28xx models found in many devices,
// including the Espressif ESP32, ESP32-S2, and ESP32-C3 development boards.

// Also demonstrates how to take advantage of the Eve HomeKit App's ability to render
// a generic custom Characteristic.  The sketch uses a custom Characterstic to create
// a "selector" button that enables to the user to select which special effect to run

#include "HomeSpan.h"
#include "extras/Pixel.h"                       // include the HomeSpan Pixel class

// IMPORTANT:  YOU LIKELY WILL NEED TO CHANGE THE PIN NUMBERS BELOW TO MATCH YOUR SPECIFIC ESP32/S2/C3 BOARD

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #define NEOPIXEL_PIN    9 
  
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  #define NEOPIXEL_PIN    7
  
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #define NEOPIXEL_PIN    21  
  
#endif

#define NUM_PIXELS        60   // number of RGBW Pixels in Pixel Strand

CUSTOM_CHAR(Selector, 00000001-0001-0001-0001-46637266EA00, PR+PW+EV, UINT8, 1, 1, 5, false);      // create Custom Characteristic to "select" special effects via Eve App

Pixel::Color BLANK=Pixel::RGB(0,0,0);

///////////////////////////////

struct Pixel_Strand : Service::LightBulb {      // Addressable RGBW Pixel Strand of nPixel Pixels

  struct SpecialEffect {
    Pixel_Strand *px;
    const char *name;

    SpecialEffect(Pixel_Strand *px, const char *name){
      this->px=px;
      this->name=name;
      Serial.printf("Adding Effect %d: %s\n",px->Effects.size()+1,name);   
    }
    
    virtual void init(){}
    virtual uint32_t update(){return(60000);}
    virtual int requiredBuffer(){return(0);}
  };  
 
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{100,true};
  Characteristic::Brightness V{100,true};
  Characteristic::Selector effect{1,true};

  vector<SpecialEffect *> Effects;
  
  Pixel *pixel; 
  int nPixels;                                 
  Pixel::Color *colors;
  uint32_t alarmTime;
  
  Pixel_Strand(int pin, int nPixels) : Service::LightBulb(){

    pixel=new Pixel(pin,true);                // creates RGBW pixel LED on specified pin using default timing parameters suitable for most SK68xx LEDs
    this->nPixels=nPixels;                    // store number of Pixels in Strand

    Effects.push_back(new ManualControl(this));
    Effects.push_back(new KnightRider(this));
    Effects.push_back(new Random(this));
    Effects.push_back(new Twinkle(this));
    Effects.push_back(new RaceTrack(this));

    effect.setUnit("");                       // configures custom "Selector" characteristic for use with Eve HomeKit
    effect.setDescription("Color Effect");
    effect.setRange(1,Effects.size(),1);

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%

    int bufSize=0;
    
    for(int i=0;i<Effects.size();i++)
      bufSize=Effects[i]->requiredBuffer()>bufSize?Effects[i]->requiredBuffer():bufSize;

    colors=(Pixel::Color *)calloc(bufSize,sizeof(Pixel::Color));   // storage for dynamic pixel pattern

    Serial.printf("\nConfigured Pixel_Strand on pin %d with %d pixels and %d effects.  Color buffer = %d pixels\n\n",pin,nPixels,Effects.size(),bufSize);

    update();
  }

  boolean update() override {

    if(!power.getNewVal()){
      pixel->set(BLANK,nPixels);
    } else {
      Effects[effect.getNewVal()-1]->init();
      alarmTime=millis()+Effects[effect.getNewVal()-1]->update();
      if(effect.updated())
        Serial.printf("Effect changed to: %s\n",Effects[effect.getNewVal()-1]->name);
    }
    
    return(true);  
  }

  void loop() override {

    if(millis()>alarmTime && power.getVal())
      alarmTime=millis()+Effects[effect.getNewVal()-1]->update();
    
  }

//////////////

  struct KnightRider : SpecialEffect {

    int phase=0;
    int dir=1;
  
    KnightRider(Pixel_Strand *px) : SpecialEffect{px,"KnightRider"} {}

    void init() override {
      float level=px->V.getNewVal<float>();
      for(int i=0;i<px->nPixels;i++,level*=0.8){
        px->colors[px->nPixels+i-1]=Pixel::HSV(px->H.getNewVal<float>(),px->S.getNewVal<float>(),level);
        px->colors[px->nPixels-i-1]=px->colors[px->nPixels+i-1];      
      }
    }

    uint32_t update() override {
      px->pixel->set(px->colors+phase,px->nPixels);
      if(phase==px->nPixels-1)
        dir=-1;
      else if(phase==0)
        dir=1;
      phase+=dir;
      return(20);      
    }

    int requiredBuffer() override {return(px->nPixels*2-1);}
 
  };
  
//////////////

  struct ManualControl : SpecialEffect {
  
    ManualControl(Pixel_Strand *px) : SpecialEffect{px,"Manual Control"} {}

    void init() override {px->pixel->set(Pixel::HSV(px->H.getNewVal<float>(),px->S.getNewVal<float>(),px->V.getNewVal<float>()),px->nPixels);}
  };

//////////////

  struct Random : SpecialEffect {
  
    Random(Pixel_Strand *px) : SpecialEffect{px,"Random"} {}

    uint32_t update() override {
      for(int i=0;i<px->nPixels;i++)
        px->colors[i]=Pixel::HSV((esp_random()%6)*60,100,px->V.getNewVal<float>());
      px->pixel->set(px->colors,px->nPixels);
      return(1000);
    }

    int requiredBuffer() override {return(px->nPixels);}
  };


///////////////////////////////

  struct Twinkle : SpecialEffect {

    int8_t *dir;
 
    Twinkle(Pixel_Strand *px) : SpecialEffect{px,"Twinkle"} {
      dir=(int8_t *)calloc(px->nPixels,sizeof(int8_t));
    }

    void init() override {
      for(int i=0;i<px->nPixels;i++){
        px->colors[i]=BLANK;
        dir[i]=0;
      }
    }

    uint32_t update() override {
      for(int i=0;i<px->nPixels;i++){
        if(px->colors[i]==Pixel::RGB(0,0,0,0)){
          if(esp_random()%200==0)
            dir[i]=15;
          else
            dir[i]=0;
        } else
        if(px->colors[i]==Pixel::RGB(0,0,0,255) || esp_random()%10==0){
          dir[i]=-15;
        }
        px->colors[i]+=Pixel::RGB(0,0,0,dir[i]);
      }
      px->pixel->set(px->colors,px->nPixels);
    return(50);  
    }

    int requiredBuffer() override {return(px->nPixels);}
 
  };

///////////////////////////////

  struct RaceTrack : SpecialEffect {

    int H=0;
    int phase=0;
    int dir=1;

    RaceTrack(Pixel_Strand *px) : SpecialEffect{px,"RaceTrack"} {}

    uint32_t update() override {
      for(int i=0;i<px->nPixels;i++){
        if(i==phase)
          px->colors[i]=Pixel::HSV(H,100,px->V.getNewVal<float>());
        else if(i==px->nPixels-1-phase)
          px->colors[i]=Pixel::HSV(H+180,100,px->V.getNewVal<float>());
        else
          px->colors[i]=BLANK;
      }

      px->pixel->set(px->colors,px->nPixels);
      phase=(phase+dir)%px->nPixels;
      
      if(phase==0){
        dir=1;
        H=(H+10)%360;
      }
      else if(phase==px->nPixels-1){
        dir=-1;
        H=(H+10)%360;
      }
            
      return(20);
    }

    int requiredBuffer() override {return(px->nPixels);}
    
  };

///////////////////////////////

};

///////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.begin(Category::Lighting,"Holiday Lights");                     
      
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Holiday Lights");
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::SerialNumber("123-ABC");
      new Characteristic::Model("NeoPixel RGBW LEDs");
      new Characteristic::FirmwareRevision("1.0");
      new Characteristic::Identify();

  new Service::HAPProtocolInformation();
    new Characteristic::Version("1.1.0");

  new Pixel_Strand(NEOPIXEL_PIN,NUM_PIXELS);

}

///////////////////////////////

void loop() {
  homeSpan.poll();
}

///////////////////////////////
