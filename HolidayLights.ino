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

#include "HomeSpan.h"
#include "extras/Pixel.h"                       // include the HomeSpan Pixel class

CUSTOM_CHAR(Selector, 00000001-0001-0001-0001-46637266EA00, PR+PW+EV, UINT8, 1, 1, 5, false);      // create Custom Characteristic to "select" special effects via Eve App

///////////////////////////////

struct Pixel_Light : Service::LightBulb {      // Addressable RGB Pixel
 
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  Pixel *pixel; 
  
  Pixel_Light(int pin) : Service::LightBulb(){

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new Pixel(pin);                     // creates pixel LED on specified pin using default timing parameters suitable for most SK68xx LEDs
    update();                                 // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();
    
    float h=H.getNewVal<float>();       // range = [0,360]
    float s=S.getNewVal<float>();       // range = [0,100]
    float v=V.getNewVal<float>();       // range = [0,100]

    pixel->setHSV(h*p, s*p, v*p);       // sets pixel to HSV colors
          
    return(true);  
  }
};

///////////////////////////////

struct Pixel_Strand : Service::LightBulb {      // Addressable RGB Pixel Strand of nPixel Pixels - Knight Rider Effect

  struct SpecialEffect {
    Pixel_Strand *px;

    SpecialEffect(Pixel_Strand *px, String name){
      this->px=px;
      Serial.printf("Adding Effect %d: %s\n",px->Effects.size()+1,name.c_str());   
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
  uint32_t *colors;
  uint32_t alarmTime;
  
  Pixel_Strand(int pin, int nPixels) : Service::LightBulb(){

    pixel=new Pixel(pin);                     // creates pixel LED on specified pin using default timing parameters suitable for most SK68xx LEDs
    this->nPixels=nPixels;                    // store number of Pixels in Strand

    Effects.push_back(new ManualControl(this));
    Effects.push_back(new KnightRider(this));

    effect.setUnit("");                       // configures custom "Selector" characteristic for use with Eve HomeKit
    effect.setDescription("Color Effect");
    effect.setRange(1,Effects.size()+3,1);

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%

    int bufSize=0;
    
    for(int i=0;i<Effects.size();i++)
      bufSize=Effects[i]->requiredBuffer()>bufSize?Effects[i]->requiredBuffer():bufSize;

    colors=(uint32_t *)calloc(bufSize,sizeof(uint32_t));   // storage for dynamic pixel pattern

    Serial.printf("Configured Pixel_Strand on pin %d with %d pixels and %d effects.  Color buffer = %d pixels\n\n",pin,nPixels,Effects.size(),bufSize);

    update();
  }

  boolean update() override {

    if(!power.getNewVal()){
      pixel->setRGB(0,0,0,nPixels);
    } else {
      Effects[effect.getNewVal()-1]->init();
      alarmTime=millis()+Effects[effect.getNewVal()-1]->update();
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
      for(int i=0;i<px->nPixels;i++,level/=2.5){
        px->colors[px->nPixels+i-1]=px->pixel->getColorHSV(px->H.getNewVal<float>(),px->S.getNewVal<float>(),level);
        px->colors[px->nPixels-i-1]=px->colors[px->nPixels+i-1];      
      }
    }

    uint32_t update() override {
      px->pixel->setColors(px->colors+phase,px->nPixels);
      if(phase==7)
        dir=-1;
      else if(phase==0)
        dir=1;
      phase+=dir;
      return(80);      
    }

    int requiredBuffer() override {return(px->nPixels*2-1);}
 
  };
  
//////////////

  struct ManualControl : SpecialEffect {
  
    ManualControl(Pixel_Strand *px) : SpecialEffect{px,"Manual Control"} {}

    void init() override {px->pixel->setHSV(px->H.getNewVal<float>(),px->S.getNewVal<float>(),px->V.getNewVal<float>(),px->nPixels);}
  };

};

///////////////////////////////

void setup() {
  
  Serial.begin(115200);
 
  homeSpan.begin(Category::Lighting,"RGB Pixels");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("RGB Pixel");
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::SerialNumber("123-ABC");
      new Characteristic::Model("SK68 LED");
      new Characteristic::FirmwareRevision("1.0");
      new Characteristic::Identify();

  new Service::HAPProtocolInformation();
    new Characteristic::Version("1.1.0");

  new Pixel_Light(8);                         // create single Pixel attached to pin 8
      
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Pixel Strand");
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::SerialNumber("123-ABC");
      new Characteristic::Model("8-LED NeoPixel");
      new Characteristic::FirmwareRevision("1.0");
      new Characteristic::Identify();

  new Service::HAPProtocolInformation();
    new Characteristic::Version("1.1.0");

  new Pixel_Strand(1,8);

}

///////////////////////////////

void loop() {
  homeSpan.poll();
}

///////////////////////////////
