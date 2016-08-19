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
String startupMsgs;
volatile int msgsSent;
volatile int stps = 0;

void getInfo(String item, YunClient client){
  //getting info from files for position tracking and control
  int itemi;
  int found =1;
  File fFile;
 if(item == "pos"){
  itemi=1;
   if(FileSystem.exists("/mnt/sda1/position.txt")) readLines(FileSystem.open("/mnt/sda1/position.txt"),itemi);
   else {
    startupMsgs+=item+" file not found. Please initialize manually before continuing</br>";
    client.print(item+" file not found</br>");
    found=0;
   }
 }
 if(item == "rng"){
  itemi=2;
   if(FileSystem.exists("/mnt/sda1/range.txt")) readLines(FileSystem.open("/mnt/sda1/range.txt"),itemi);
   else {
    startupMsgs+=item+" file not found. Please initialize manually before continuing</br>";
    client.print(item+" file not found</br>");
    found=0;
   }
 }
 if(item == "algn"){
  itemi=3;
   if(FileSystem.exists("/mnt/sda1/alignment.txt")) readLines(FileSystem.open("/mnt/sda1/alignment.txt"),itemi);
   else {
    startupMsgs+=item+" file not found. Please initialize manually before continuing</br>";
    client.print(item+" file not found</br>");
    found=0;
   }
 }
}
void readLines(File fFile, int itemi) {
  //read the first line in the file. Delimiter is L
   String output = "";
    int line =0;
    char chr;
    int charsread=0;
    while (line<3) {
      charsread+=1;
      chr = (char)fFile.read();
      if(chr != 'L') output+=chr;
      else {
        output.trim();
        //read text file into the appropriate program variable
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
      if(charsread>100){
        break;
      }
    }
    fFile.close();
}
void setInfo(int item[], String itm, YunClient client){
  //Prep data for writing, then delete previous file if it exists and then write new file
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


void setup() {
  msgsSent=0;
  startupMsgs="";
  //the above pertain to motion tracking files...see functions invovled with reading files
  Serial.begin(9600);
  // Bridge startup
  pinMode(8, OUTPUT);//c dir
  pinMode(9, OUTPUT);//a dir
  pinMode(10, OUTPUT);//b dir
  pinMode(12, OUTPUT);//c pulse
  pinMode(4, OUTPUT);//a pulse
  pinMode(6, OUTPUT);//b pulse
  pinMode(A2,OUTPUT);// c enable
  pinMode(A3,OUTPUT);// b enable
  pinMode(A4,OUTPUT);// a enable
  digitalWrite(A2,HIGH);// disable c
  digitalWrite(A3,HIGH);// disable b
  digitalWrite(A4,HIGH);// disable a
  Bridge.begin();
  server.listenOnLocalhost();
  server.begin();
  YunClient client = server.accept();
  //try and get the saved position, range and alignment data
}

void stepMotor(String Motor, long steps, String dir, YunClient client) {
  stps=0;
  int mot1;
  int mot2;
  int mot3;
  static unsigned int pulsePort[] = { 0b10111111/*6*/, 0b11101111/*4*/, 0b01111111/*7*/ }; //PORTD, Digital Pins 12,4,6 (C,A,B)
  static unsigned int dirPort[] = { 0b11101111/*4*/, 0b11011111/*5*/, 0b10111111/*6*/ }; //PORTB, Digital Pins 8,9,10 (C,A,B)
  //cases A,B,C,ALL,AB,AC,BC, CS= AB together, but opposite directions (for Pitch)
  DDRD = 0b11111011;//setup port D for output
  DDRB = 0b11111111;//setup port B for output
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
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[1]||mot3<0 || mot3>rng[2]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A3,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
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
  if (String(Motor) == "A") {
    mot1=pos[0]+sgn*steps;
    if( mot1<0 || mot1>rng[0]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[1]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[0]+=sgn;
      }
    }
  }
  if (String(Motor) == "B") {
    mot2=pos[1]+sgn*steps;
    if( mot2<0 || mot2>rng[1]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A3,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[2];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[2]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[2];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[1]+=sgn;
      }
    }
  }
  if (String(Motor) == "C") {
    mot3=pos[2]+sgn*steps;
    if( mot3<0 || mot3>rng[2]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[0]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[2]+=sgn;
      }
    }
  }
/*
  if (String(Motor) == "AB") {
    mot1=pos[0]+sgn*steps;
    mot2=pos[1]+sgn*steps;
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[1]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A3,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[1] & pulsePort[2]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[1] & pulsePort[2]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1] & pulsePort[2]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[0]+=sgn;
          pos[1]+=sgn;
      }
    }
  }
*/
  if (String(Motor) == "AB") {
    mot1=pos[0]+sgn*steps;
    mot2=pos[1]+sgn*steps;
    mot3=pos[2]-sgn*steps;
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[1] || mot3<0 || mot3>rng[2]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A3,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[1] & pulsePort[2]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[1] & pulsePort[2]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1] & pulsePort[2]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          if (String(dir) == "+") { //DIR HIGH
            PORTB |=  ~(dirPort[0] & dirPort[1] & dirPort[2]);
          }
          if (String(dir) == "-") { //DIR LOW
            PORTB &= dirPort[0] & dirPort[1] & dirPort[2];
          }
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[0]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          if (String(dir) == "-") { //DIR HIGH
            PORTB |=  ~(dirPort[0] & dirPort[1] & dirPort[2]);
          }
          if (String(dir) == "+") { //DIR LOW
            PORTB &= dirPort[0] & dirPort[1] & dirPort[2];
          }
          stps += 1;
          pos[0]+=sgn;
          pos[1]+=sgn;
	  pos[2]-=sgn;
      }
    }
  }
  if (String(Motor) == "BC") {
    mot2=pos[1]+sgn*steps;
    mot3=pos[2]+sgn*steps;
    if( mot2<0 || mot2>rng[1]||mot3<0 || mot3>rng[2]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A3,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[0] & pulsePort[2]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[0] & pulsePort[2]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0] & pulsePort[2]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[1]+=sgn;
          pos[2]+=sgn;
      }
    }
  }
  if ( String(Motor) == "AC") {
    mot1=pos[0]+sgn*steps;
    mot3=pos[2]+sgn*steps;
    if( mot1<0 || mot1>rng[0] ||mot3<0 || mot3>rng[2]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A2,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps; i++) {
          PORTD = PORTD & pulsePort[0] & pulsePort[1]; //Pulse LOW
          PORTD = PORTD | ~(pulsePort[0] & pulsePort[1]); //Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[0] & pulsePort[1]; //Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          stps += 1;
          pos[0]+=sgn;
          pos[2]+=sgn;
      }
    }
  }
  if (String(Motor) == "CS") {
    mot1=pos[0]+sgn*steps/2;
    mot2=pos[1]-sgn*steps/2;
    if( mot1<0 || mot1>rng[0] ||mot2<0 || mot2>rng[1]){
      client.print("Motion exceedes range. Try moving less </br>");
    }
    else{
      digitalWrite(A3,LOW);
      digitalWrite(A4,LOW);
      for (int i = 0; i < steps/2; i++) {
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[1]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[1];//Pulse LOW
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          if (String(dir) == "+") { //DIR HIGH
            PORTB |=  ~(dirPort[0] & dirPort[1] & dirPort[2]);
          }
          if (String(dir) == "-") { //DIR LOW
            PORTB &= dirPort[0] & dirPort[1] & dirPort[2];
          }
          PORTD = PORTD & pulsePort[2];//Pulse LOW
          PORTD = PORTD | ~(pulsePort[2]);//Pulse HIGH
          for(int k = 0; k<1000;k++){asm("");}//delay(1);
          PORTD = PORTD & pulsePort[2];//Pulse LOW
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
  client.print("Motion Finished; ");
  digitalWrite(A2,HIGH);// disable c
  digitalWrite(A3,HIGH);// disable b
  digitalWrite(A4,HIGH);// disable a
  client.print("Saving Position</br>");
  setInfo(pos,"pos",client);
  String moString;
  moString = getTimeStamp() + " -- Motion Finished -- " + String(stps) + " step(s)\n";
  moString += "\tNew Position: " + String(pos[0]) + " " + String(pos[1]) + " " + String(pos[2]);
  File dataFile = FileSystem.open("/mnt/sda1/datalog.txt", FILE_APPEND);
  if (dataFile) {
    dataFile.println(moString);
    dataFile.close();
  }
}

void process(YunClient client) {
  if(msgsSent){
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
    client.print(command[0]); //Dir
    client.print(" :");
    client.print(command[1]); //Steps
    client.print(" :");
    client.print(command[2]); //Motors
    client.print("</br>");

    File dataFile = FileSystem.open("/mnt/sda1/datalog.txt", FILE_APPEND);
    String outString;
    outString = getTimeStamp()+ " -- Commands recieved: ";
    outString += command[0] + " :" + command[1] + " :" + command[2];  
    if (dataFile) {
      dataFile.println(outString);
      dataFile.close();  
    } 

    String newInfo = "";
 
    //commands[0,1,2] = Dir, Steps, Motor
    if(command[0].indexOf("getPos")>=0){
      getInfo("pos",client);
      client.print("Position: A-");
      client.print(String(pos[0]));
      client.print("  B-");
      client.print(String(pos[1]));
      client.print("  C-");
      client.print(String(pos[2]));
      client.print("</br>");
      newInfo += String(pos[0]) + " " + String(pos[1]) + " " + String(pos[2]);
    }
    else if(command[0].indexOf("setAlgn")>=0){
      setInfo(pos,"algn",client);
    }
    else if(command[0].indexOf("setRng")>=0){
      setInfo(pos,"rng",client);
    }
    else if(command[0].indexOf("getAlgn")>=0){
      getInfo("algn",client);
      client.print("Alignment: A-");
      client.print(String(algn[0]));
      client.print("  B-");
      client.print(String(algn[1]));
      client.print("  C-");
      client.print(String(algn[2]));
      client.print("</br>");
      newInfo += String(algn[0]) + " " + String(algn[1]) + " " + String(algn[2]);
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
      newInfo += String(rng[0]) + " " + String(rng[1]) + " " + String(rng[2]);
    }
    //below is the testing loop
    else if(command[0].indexOf("cta")>=0){
      for(int gf =0; gf<50; gf++){
        client.print("loop: ");
        client.println(gf);
        stepMotor(String("All"), long(5000), String("+"), client);
        stepMotor(String("All"), long(10000), String("-"), client);
        stepMotor(String("All"), long(10000), String("+"), client);
        stepMotor(String("All"), long(5000), String("-"), client);
      }
    }
    else {
      stepMotor( command[2], long(command[1].toFloat()), command[0], client );
    }
    if (newInfo.length() > 0) {
       File dataFileX = FileSystem.open("/mnt/sda1/datalog.txt", FILE_APPEND);
       if (dataFileX) {
         dataFileX.println(newInfo);
         dataFileX.close();  
       } 
    }
  }
  else {
     getInfo("rng",client);
     getInfo("pos",client);
     getInfo("algn",client);
     client.print("If needed please complete startup manually. See below </br>");
     client.print(startupMsgs);
     msgsSent=1;

     // Record when it started up and initial position, range, alignment values
     String outString;
     outString = "\n\nStarting up Motion Control --- " + getTimeStamp();
     outString += "\n\tPosition loaded:  " + String(pos[0]) + " " + String(pos[1]) + " " + String(pos[2]);
     outString += "\n\tRange loaded:  " + String(rng[0]) + " " + String(rng[1]) + " " + String(rng[2]);
     outString += "\n\tAlignment loaded:  " + String(algn[0]) + " " + String(algn[1]) + " " + String(algn[2]);
     File dataFile = FileSystem.open("/mnt/sda1/datalog.txt", FILE_APPEND);
     if (dataFile) {
       dataFile.println(outString);
       dataFile.close();
     }
     else {
       client.print("No Log file found, manually create it and restart.  (/mnt/sda1/datalog.txt) </br>");
     }
  }
}

// This function return a string with the time stamp
String getTimeStamp() {
  String result;
  Process time;
  // date is a command line utility to get the date and the time
  // in different formats depending on the additional parameter
  time.begin("date");
  time.addParameter("+%D-%T");  // parameters: D for the complete date mm/dd/yy
  //             T for the time hh:mm:ss
  time.run();  // run the command
  // read the output of the command
  while (time.available() > 0) {
    char c = time.read();
    if (c != '\n')
      result += c;
  }
  return result;
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();
  if (client) {
    process(client);
    client.stop();
  };
}

