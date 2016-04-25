 #include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <FileIO.h>
// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;
int pos[3];
int rng[3];
int algn[3];
volatile int AINT = 0;
volatile int stps = 0;

void getInfo(String item, YunClient client){
  int itemi;
  int found =1;
  File fFile;
 if(item == "pos"){
  itemi=1;
   if(FileSystem.exists("/mnt/sda1/position.txt")) readLines(FileSystem.open("/mnt/sda1/position.txt"),itemi);
   else {
    client.print(item+" file not found</br>");
    found=0;
   }
 }
 if(item == "rng"){
  itemi=2;
   if(FileSystem.exists("/mnt/sda1/range.txt")) readLines(FileSystem.open("/mnt/sda1/range.txt"),itemi);
   else {
    client.print(item+" file not found</br>");
    found=0;
   }
 }
 if(item == "algn"){
  itemi=3;
   if(FileSystem.exists("/mnt/sda1/alignment.txt")) readLines(FileSystem.open("/mnt/sda1/alignment.txt"),itemi);
   else {
    client.print(item+" file not found</br>");
    found=0;
   }
 }
}
void readLines(File fFile, int itemi) {
   String output = "";
    int line =0;
    char chr;
    while (line<3) {
      chr = (char)fFile.read();
      if(chr != 'L') output+=chr;
      else {
        output.trim();
        switch(itemi){
          case 1:
            pos[line]=output.toInt();
            break;
          case 2:
            rng[line]=output.toInt();
            break;
          case 3:
            algn[line]=output.toInt();
           break;  
        }
        output = "";
        line++;
      }
    }
    fFile.close();
}
void setInfo(int item[], String itm, YunClient client){
  String info ="";
  info +=String(item[0]);
  info +='L';
  info +=String(item[1]);
  info +='L';
  info +=String(item[2]);
  info +='L';
 if(itm == "pos"){
   if(FileSystem.exists("/mnt/sda1/position.txt")) FileSystem.remove("/mnt/sda1/position.txt");
   File fFile = FileSystem.open("/mnt/sda1/position.txt",FILE_WRITE);
   fFile.print(info);
   fFile.close();
 }
 if(itm == "rng"){
   if(FileSystem.exists("/mnt/sda1/range.txt")) FileSystem.remove("/mnt/sda1/range.txt");
   File fFile = FileSystem.open("/mnt/sda1/range.txt",FILE_WRITE);
   fFile.print(info);
   fFile.close();
 }
 if(itm == "algn"){
   if(FileSystem.exists("/mnt/sda1/alignment.txt")) FileSystem.remove("/mnt/sda1/alignment.txt");
   File fFile = FileSystem.open("/mnt/sda1/alignment.txt",FILE_WRITE);
   fFile.print(info);
   fFile.close();
 }
}

void range(YunClient client){
  client.print("Moving motors to max +Z; ");
  stepMotor("A",500,"+",client);
  stepMotor("B",500,"+",client);
  stepMotor("C",500,"+",client);
  client.print("Finding Motor A range; ");
  stepMotor("A",500,"-",client);
  pos[0]=stps;
  client.print("Finding Motor B range; ");
  stepMotor("B",500,"-",client);
  pos[1]=stps;
  client.print("Finding Motor C range; ");
  stepMotor("C",500,"-",client);
  pos[2]=stps;
  String rng ="";
  rng +=pos[0];
  rng +='L';
  rng +=pos[1];
  rng +='L';
  rng +=pos[2];
  rng +='L';
  if(FileSystem.exists("mnt/sda1/range.txt")) FileSystem.remove("/mnt/sda1/range.txt");
  File fFile = FileSystem.open("/mnt/sda1/range.txt",FILE_WRITE);
  setInfo(pos,"pos",client);
  fFile.print(rng);
  fFile.close();
}

void setup() {
  Serial.begin(9600);
  // Bridge startup
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(A2,OUTPUT);
  pinMode(A3,OUTPUT);
  pinMode(A4,OUTPUT);
  digitalWrite(A2,HIGH);
  digitalWrite(A3,HIGH);
  digitalWrite(A4,HIGH);
  pinMode(3, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(3), rupt, RISING);
  Bridge.begin();
  server.listenOnLocalhost();
  server.begin();
  YunClient client = server.accept();
  getInfo("rng",client);
  getInfo("pos",client);
  getInfo("algn",client);
}
void rupt() {
  AINT = 1;
}

void stepMotor(String Motor, long steps, String dir, YunClient client) {
  AINT=0;
  stps=0;
  int mot1;
  int mot2;
  int mot3;
  static unsigned int pulsePort[] = { 0b10111111/*6*/, 0b11101111/*4*/, 0b01111111/*7*/ }; //PORTD Pins 12,4,6 (A,B,C)
  static unsigned int dirPort[] = { 0b11101111/*4*/, 0b11011111/*5*/, 0b10111111/*6*/ }; //PORTB Pins 8,9,10 (A,B,C)
  //cases A,B,C,ALL,AB,AC,BC
  DDRD = 0b11111011;
  DDRB = 0b11111111;
  int sgn =1;
  if (String(dir) == "-") { //DIR HIGH
    PORTB |=  ~(dirPort[0] & dirPort[1] & dirPort[2]);
    sgn=-1;
  }
  if (String(dir) == "+") { //DIR LOW
    PORTB &= dirPort[0] & dirPort[1] & dirPort[2];
  }
  
  if (String(Motor) == "All") {
    mot1=pos[0]+sgn*steps;
    mot2=pos[1]+sgn*steps;
    mot3=pos[2]+sgn*steps;
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[0]||mot3<0 || mot3>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A3,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
        if(AINT==0){
          PORTD &= pulsePort[0] & pulsePort[1] & pulsePort[2]; //Pulse LOW
          PORTD |= ~(pulsePort[0] & pulsePort[1] & pulsePort[2]);
          //delay(1);
          for(int k = 0; k<1000;k++){asm("");}
          PORTD &= (pulsePort[0] & pulsePort[1] & pulsePort[2]); //Pulse LOW
          //delay(1);
          for(int k = 0; k<1000;k++){asm("");}
          stps += 1;
          pos[0]+=sgn;
          pos[1]+=sgn;
          pos[2]+=sgn;
        }
      }
    }
  }
  if (String(Motor) == "A") {
    mot1=pos[0]+sgn*steps;
    if( mot1<0 || mot1>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      for (int i = 0; i < steps; i++) {
        if(AINT==0){
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[0]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[0]+=sgn;
        }
      }
    }
  }
  if (String(Motor) == "B") {
    mot2=pos[1]+sgn*steps;
    if( mot2<0 || mot2>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
       if(AINT==0){
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[1]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[1]+=sgn;
        }
      }
    }
  }
  if (String(Motor) == "C") {
    mot3=pos[2]+sgn*steps;
    if( mot3<0 || mot3>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A3,LOW);
      for (int i = 0; i < steps; i++) {
        if(AINT==0){
          PORTD = PORTD & pulsePort[2];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[2]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[2];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[2]+=sgn;
        }
      }
    }
  }
  if (String(Motor) == "AB") {
    mot1=pos[0]+sgn*steps;
    mot2=pos[1]+sgn*steps;
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
        if(AINT==0){
          PORTD = PORTD & pulsePort[0] & pulsePort[1]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[0] & pulsePort[1]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0] & pulsePort[1]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[0]+=sgn;
          pos[1]+=sgn;
        }
      }
    }
  }
  if (String(Motor) == "BC") {
    mot2=pos[1]+sgn*steps;
    mot3=pos[2]+sgn*steps;
    if( mot2<0 || mot2>rng[0]||mot3<0 || mot3>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A3,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
        if(AINT==0){
          PORTD = PORTD & pulsePort[1] & pulsePort[2]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[1] & pulsePort[2]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1] & pulsePort[2]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[1]+=sgn;
          pos[2]+=sgn;
        }
      }
    }
  }
  if ( String(Motor) == "AC") {
    mot1=pos[0]+sgn*steps;
    mot3=pos[2]+sgn*steps;
    if( mot1<0 || mot1>rng[0] ||mot3<0 || mot3>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A3,LOW);
      for (int i = 0; i < steps; i++) {
        if(AINT==0){
          PORTD = PORTD & pulsePort[0] & pulsePort[2]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[0] & pulsePort[2]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0] & pulsePort[2]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[0]+=sgn;
          pos[2]+=sgn;
        }
      }
    }
  }
  if (String(Motor) == "CS") {
    mot1=pos[0]+sgn*steps/2;
    mot2=pos[1]-sgn*steps/2;
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps/2; i++) {
        if(AINT==0){
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[0]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          if (String(dir) == "+") { //DIR HIGH
            PORTB |=  ~(dirPort[0] & dirPort[1] & dirPort[2]);
          }
          if (String(dir) == "-") { //DIR LOW
            PORTB &= dirPort[0] & dirPort[1] & dirPort[2];
          }
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[1]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          if (String(dir) == "-") { //DIR HIGH
            PORTB |=  ~(dirPort[0] & dirPort[1] & dirPort[2]);
          }
          if (String(dir) == "+") { //DIR LOW
            PORTB &= dirPort[0] & dirPort[1] & dirPort[2];
          }
          stps += 1;
          pos[0]+=sgn;
          pos[1]-=sgn;
        }
      }
    }
  }
  client.print("Motion Finished; ");
  client.print("Interruptions: ");
    if(AINT == 1){
      client.print(String(stps));
      client.print(" steps taken before interruption.; ");
    }
    else {
      client.print("None; ");
    }
    digitalWrite(A2,HIGH);
    digitalWrite(A3,HIGH);
    digitalWrite(A4,HIGH);
    client.print("Saving Position</br>");
    setInfo(pos,"pos",client);
}

void process(YunClient client) {
  String command[3];
  String commands;
  commands = client.readStringUntil('/');
  commands.trim();
  command[0] = commands;
  commands = client.readStringUntil('/');
  commands.trim();
  command[1] = commands;
  commands = client.readStringUntil('/');
  commands.trim();
  command[2] = commands;
  client.print("Commands recieved: ");
 // client.print("Dir ");
  client.print(command[0]);
  client.print(" :");
 // client.print("Steps ");
  client.print(command[1]);
  client.print(" :");
  //client.print("Motors ");
  client.print(command[2]);
  client.print("</br>");
  //commands[0,1,2] = Dir, Steps, Motor
  if(command[0].indexOf("interruptions")>=0){
    client.print(AINT);
  }
  else if(command[0].indexOf("getPos")>=0){
    getInfo("pos",client);
    client.print("Position: A-");
    client.print(String(pos[0]));
    client.print("  B-");
    client.print(String(pos[1]));
    client.print("  C-");
    client.print(String(pos[2]));
    client.print("</br>");
  }
  else if(command[0].indexOf("setAlgn")>=0){
    setInfo(pos,"algn",client);
  }
  else if(command[0].indexOf("setRng")>=0){
    setInfo(pos,"rng",client);
  }
  else if(command[0].indexOf("getAlgn")>=0){
    client.print("Alignment: A-");
    client.print(String(algn[0]));
    client.print("  B-");
    client.print(String(algn[1]));
    client.print("  C-");
    client.print(String(algn[2]));
    client.print("</br>");
  }
  else if(command[0].indexOf("getRng")>=0){
    getInfo("rng",client);
    client.print("Range: A-");
    client.print(String(rng[0]));
    client.print("  B-");
    client.print(String(rng[1]));
    client.print("  C-");
    client.print(String(rng[2]));
    client.print("</br>");
  }
  else if(command[0].indexOf("findRng")>=0){
    range(client);
  }
  else if(command[0].indexOf("cta")>=0){
    for(int gf =0; gf<59; gf++){
      client.print("loop: ");
      client.println(gf);
    stepMotor(String("All"), long(5000), String("+"), client);
    stepMotor(String("All"), long(10000), String("-"), client);
    stepMotor(String("All"), long(10000), String("+"), client);
    stepMotor(String("All"), long(5000), String("-"), client);
    
    stepMotor(String("CS"), long(10000), String("+"), client);
    stepMotor(String("CS"), long(20000), String("-"), client);
    stepMotor(String("CS"), long(20000), String("+"), client);
    stepMotor(String("CS"), long(10000), String("-"), client);
    
    stepMotor(String("AB"), long(5000), String("+"), client);
    stepMotor(String("AB"), long(10000), String("-"), client);
    stepMotor(String("AB"), long(10000), String("+"), client);
    stepMotor(String("AB"), long(5000), String("-"), client);
    
    stepMotor(String("A"), long(5000), String("+"), client);
    stepMotor(String("A"), long(10000), String("-"), client);
    stepMotor(String("A"), long(10000), String("+"), client);
    stepMotor(String("A"), long(5000), String("-"), client);

    stepMotor(String("B"), long(5000), String("+"), client);
    stepMotor(String("B"), long(10000), String("-"), client);
    stepMotor(String("B"), long(10000), String("+"), client);
    stepMotor(String("B"), long(5000), String("-"), client);

    stepMotor(String("C"), long(5000), String("+"), client);
    stepMotor(String("C"), long(10000), String("-"), client);
    stepMotor(String("C"), long(10000), String("+"), client);
    stepMotor(String("C"), long(5000), String("-"), client);
    }
  }
  else {
    stepMotor( command[2], long(command[1].toFloat()), command[0], client );
  }
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();
  if (client) {
    process(client);
    client.stop();
  };
}

